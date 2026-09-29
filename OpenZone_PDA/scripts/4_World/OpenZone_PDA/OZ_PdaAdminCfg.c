// Аплаєри конфігів КПК для адмінської консолі ядра.
//
// ЖИВУТЬ У 4_World, а не поруч із самим конфігом: базовий OZ_AdminCfgApplier
// оголошений ядром у 4_World, і 3_Game його ще не бачить -- модулі скриптів
// компілюються ярусами (зміряно цим самим файлом 2026-08-30).

// Спільний крок трьох аплаєрів: розбір -> МІГРАЦІЯ Й ПЕРЕВІРКА -> запис.
//
// ПЕРЕВІРКА ДО ПЕРШОГО ЗАПИСУ, і в цьому вся суть класу.
//
// Було: Save(json як є) -> ServerLoad(). Save робить .bak із того, що лежало
// на диску, -- це і є єдина точка відкату адміна. Але ServerLoad читає щойно
// записаний файл, Validate тихо його лагодить (кламп межі, порожній масив,
// невідома сторінка), і лоадер, побачивши warnings > 0, ПЕРЕЗАПИСУЄ файл --
// із власним .bak. Другий бекап затирав перший: копія «як було до правки»
// зникала, а на її місце лягала «як стало після правки», тобто відкат
// вертав рівно те, від чого тікали.
//
// Тепер лагодимо ще в пам'яті, і на диск лягає вже чистий файл. ServerLoad
// не має чого переписувати, warnings = 0, .bak лишається тим, що треба.
//
// Дженерик, а не три копії: різниця між трьома аплаєрами -- ТИП, ШЛЯХ і те,
// чий ServerLoad кликати. Перші два сюди передаються, третій лишається
// однорядковим у кожного: підставити його дженериком нема чим, а
// «перечитати всі три конфіги на кожну правку одного» -- дорожче й брехливо.
class OZ_PdaCfgApply<Class T>
{
    // ОБ'ЄКТ СТВОРЮЄ ВИКЛИКАЧ, і саме тому він приїжджає параметром.
    //
    // Тут стояло голе `T tmp;` -- отже корінь виділяв серіалізатор, а він не
    // виконує ані конструктора, ані ініціалізаторів полів (шапка
    // OZ_ConfigBase ядра). Version читалось із сирої пам'яті рівно в тій
    // перевірці, що мусить відмовити файлові з майбутнього. `new T()`
    // всередині дженерика Enforce не працює надійно -- ту саму причину
    // записало ядро в OZ_ConfigLoader, -- тож кожен аплаєр створює свій
    // конкретний тип сам.
    static bool Write(T tmp, string json, string path, string tag)
    {
        string err;
        if (!JsonFileLoader<T>.LoadData(json, tmp, err) || !tmp)
        {
            OZ_Log.Warn("admin: " + tag + ".json rejected: " + err);
            return false;
        }

        // Файл із МАЙБУТНЬОГО не чіпаємо -- те саме правило, що в лоадера:
        // Migrate уміє лише `from < N`, і на старшій версії він викинув би
        // кожне поле, якого цей білд не знає.
        if (tmp.Version > tmp.LatestVersion())
        {
            OZ_Log.Warn("admin: " + tag + ".json is v" + tmp.Version.ToString() + ", newer than this build - refused");
            return false;
        }

        if (tmp.Version != tmp.LatestVersion())
        {
            if (!tmp.Migrate(tmp.Version))
            {
                OZ_Log.Warn("admin: " + tag + ".json cannot be migrated from v" + tmp.Version.ToString() + " - refused");
                return false;
            }
        }

        int warnings;
        tmp.Validate(warnings);
        if (warnings > 0)
            OZ_Log.Info("admin: " + tag + ".json fixed " + warnings.ToString() + " thing(s) before saving");

        OZ_ConfigLoader<T>.Save(path, tag, tmp);
        return true;
    }
}

class OZ_PdaTuningApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZ_PdaTuning tun = new OZ_PdaTuning();
        if (!OZ_PdaCfgApply<OZ_PdaTuning>.Write(tun, json, OZ_Const.PROFILE_DIR + "\\OZ_PDA_Tuning.json", "Tuning"))
            return false;

        OZ_PdaTuning.ServerLoad();

        // ЗАСТОСУВАННЯ -- ЦЕ НЕ ЛИШЕ ПЕРЕЧИТАТИ ФАЙЛ.
        //
        // Два числа з Tuning їдуть клієнтові пакетом синхронізації ядра, а
        // третє живе в періоді серверного таймера. Без цих двох рядків
        // «застосовано» означало «застосовано після рестарту»: тост і крок
        // маршруту лишались старими в кожного, хто вже в Зоні, а посилки
        // маячків ходили зі старим періодом до кінця запуску.
        OZ_PdaModule.RearmBeacons();

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            if (!players[i])
                continue;
            PlayerIdentity id = players[i].GetIdentity();
            if (id)
                OZ_SyncSender.Send(id, "pda tuning applied");
        }

        return true;
    }
}

class OZ_PdaProfilesApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZ_PdaProfilesConfig prof = new OZ_PdaProfilesConfig();
        if (!OZ_PdaCfgApply<OZ_PdaProfilesConfig>.Write(prof, json, OZ_PdaConst.PROFILES, "Profiles"))
            return false;

        OZ_PdaProfiles.ServerLoad();

        // Число відсіків кожного класу їде клієнтові пакетом синхронізації
        // (OZ_PdaConst.SYNC_SLOTS): з нього інвентар знає, які гнізда
        // показати. Без розсилки тут змінений ModuleSlots доходив до тих, хто
        // вже в Зоні, лише з перезаходом. Витрату живих приладів наздоганяє
        // їхній власний OnWork (OZ_PDA_Base.OZ_CfgFollow).
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            if (!players[i])
                continue;
            PlayerIdentity id = players[i].GetIdentity();
            if (id)
                OZ_SyncSender.Send(id, "pda profiles applied");
        }

        return true;
    }
}

class OZ_PdaHardwareApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZ_PdaHardwareConfig hw = new OZ_PdaHardwareConfig();
        if (!OZ_PdaCfgApply<OZ_PdaHardwareConfig>.Write(hw, json, OZ_PdaConst.HARDWARE, "Hardware"))
            return false;

        OZ_PdaHardware.ServerLoad();
        return true;
    }
}
