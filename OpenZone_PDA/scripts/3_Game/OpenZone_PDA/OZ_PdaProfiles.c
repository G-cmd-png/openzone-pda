// Профілі пристроїв: що вміє конкретний КПК.
//
// Тир задає ПРЕДМЕТ, а не гравець: ПДА новачка й ПДА долговця -- різні
// класнейми з різними наборами сторінок, лімітів і живлення.
//
// ClassNames -- список, і в ньому може стояти класнейм БУДЬ-ЯКОГО мода.
// Пізнаємо пристрій звіркою obj.GetType() зі списком, і НІКОЛИ спорідненістю
// з нашим класом: інакше адмін не зміг би навести мод на чужий предмет, а
// це половина сенсу «максимально гнучкого».

class OZ_PdaLimits
{
    // ЯЧЕЙКИ ПАМ'ЯТІ, спільні на все, що прилад тримає в собі.
    //
    // Було по стелі на кожен рід -- Markers і Notes окремо. Це вимагало від
    // МОДЕЛІ ПРИЛАДУ знати наперед про кожен майбутній рід даних: поки родів
    // три й усі свої, це працює, а перший же чужий модуль опиняється або без
    // стелі, або з полем, вписаним у чужий конфіг. Рівно та сама пастка, з
    // якої вже вийшов носій.
    //
    // Тепер одне число, і ціна однакова: мітка -- ячейка, нотатка -- ячейка,
    // частота -- ячейка, маршрут -- ячейка (він лише ПОРЯДОК зв'язків між
    // мітками, а не самі мітки; тому перенести маршрут означає перенести його
    // мітки ПЛЮС його самого).
    //
    // Друзі й групові розмови сюди НЕ входять: вони не лежать у пам'яті
    // приладу, а живуть на сервері й у мосту.
    // НУЛЬ, а не число: профіль, що не оголосив пам'ять, отримує
    // задокументоване умовчання з WARNING (ТЗ-4 R-F1.1--R-F1.3), а не тихе
    // число з іншого файлу. Два числа у двох місцях -- це те, з чого все
    // почалось.
    int Memory     = 0;
    int Friends    = 20;
    int GroupChats = 2;

    // Копія в об'єкт, який зробив скрипт (шапка OZ_ConfigBase ядра).
    //
    // Ключ, якого У ФАЙЛІ НЕМАЄ, копія переносить нулем, а не числом з
    // ініціалізатора нижче: після копії «немає у файлі» й «нуль у файлі»
    // невідрізненні. Тому ініціалізатори цих трьох полів -- ДОКУМЕНТАЦІЯ, а
    // не робочий механізм, і кожне поле мусить сказати, хто ставить йому
    // умовчання насправді:
    //
    //   Memory     -- Validate, ВГОЛОС (WARNING на ім'я профілю, R-F1.3);
    //   Friends    -- Validate, МОВЧКИ, з OZ_PdaConst.FRIENDS_DEFAULT. Нуль
    //                 тут не можна лишити як прочитався: Full() читає
    //                 Friends <= 0 як «стелі немає», тобто забутий ключ не
    //                 звужував би межу записника, а скасовував її;
    //   GroupChats -- ПОКИ ЩО НІХТО, і це не «все гаразд», а відкритий
    //                 пункт. Нуль їде до моста полем OZ_ChatAskGroup.Max, а
    //                 там він означає «без межі» -- тобто пастка та сама, що
    //                 у Friends. Не чіпаємо в цій хвилі лише тому, що нуль
    //                 тут ще й ЗАКОННЕ значення на дроті: OZ_PdaChat ставить
    //                 Max = 0 засновникові, у якого профілю немає взагалі.
    //                 Розв'язати це має лоадер, який відрізняє «немає ключа»
    //                 від нуля (та сама вимога, що в task-57b, пункт 2).
    OZ_PdaLimits Copy()
    {
        OZ_PdaLimits c = new OZ_PdaLimits();
        c.Memory     = Memory;
        c.Friends    = Friends;
        c.GroupChats = GroupChats;
        return c;
    }
}

class OZ_PdaProfile
{
    string            Id          = "";
    ref array<string> ClassNames;
    string            DisplayName = "";
    ref array<string> Pages;
    ref array<string> BatteryClassNames;
    ref OZ_PdaLimits  Limits;

    // RequiresPower, PowerDrainPerMin, Theme і PinProtectedPages ЗНЯТІ
    // (ревізія 2026-09-06). Усі чотири оголошувались, валідувались і не
    // читались ніде: живлення вирішує рушійний ComponentEnergyManager,
    // тема одна на серію (ui/tokens.json ядра), а «сторінки, що просять код
    // ще раз» ворота OZ_PdaAccess не питали ніколи. Поле, яке обіцяє
    // адмінові важіль і нічого не робить, гірше за відсутнє.

    // Скільки модульних відсіків видно на цій моделі. ГОЛОВНИЙ важіль тиру:
    // один відсік змушує вибирати між далеким зв'язком і лічильником Гейгера,
    // три дозволяють нести все. Стеля -- MODULE_SLOTS_MAX, бо слоти не
    // додаються в рантаймі.
    int ModuleSlots = 1;

    // --- замок ---
    // Через скільки хвилин після того, як пристрій прибрали з рук, він
    // замикається сам. Нуль вимикає автоблокування для цієї моделі зовсім.
    float LockAfterMinutes = 5;
    // Сервер забороняє гравцеві вимикати автоблокування. Для серверів, де
    // залутаний КПК має лишатись цінністю, а не безкоштовним трофеєм.
    bool  ForceAutoLock    = false;

    // --- запечатаний пристрій ---
    //
    // Це КПК із чужої історії: мертвого сталкера, кинутої схованки, квесту.
    // Він приходить у світ із кодом, якого НІХТО не знає -- сервер ставить
    // випадковий і нікому його не каже. Підібрати його не можна: він не
    // «складний», його просто не існує в жодній голові.
    //
    // Відкрити можна ЛИШЕ дешифратором, і тільки часом. Тому в такого КПК
    // цінність не в залізі, а в тому, що на ньому записано.
    bool  Sealed       = false;
    float CrackSeconds = 120;

    // Що на ньому вже записано, коли він з'явився у світі. Пишеться ОДИН раз,
    // при першій появі предмета -- інакше кожен рестарт відновлював би
    // стерті гравцем мітки.
    ref array<ref OZ_MapMarker> PresetMarkers;

    // s_Cfg живе весь запуск сервера, і ForClass читає ці поля на кожному
    // запиті кожної сторінки -- через години після розбору файла.
    OZ_PdaProfile Copy()
    {
        OZ_PdaProfile c = new OZ_PdaProfile();
        c.Id               = Id;
        c.DisplayName      = DisplayName;
        c.ModuleSlots      = ModuleSlots;
        c.LockAfterMinutes = LockAfterMinutes;
        c.ForceAutoLock    = ForceAutoLock;
        c.Sealed           = Sealed;
        c.CrackSeconds     = CrackSeconds;

        int i;
        c.ClassNames = new array<string>();
        if (ClassNames)
        {
            for (i = 0; i < ClassNames.Count(); i++)
                c.ClassNames.Insert(ClassNames[i]);
        }

        c.Pages = new array<string>();
        if (Pages)
        {
            for (i = 0; i < Pages.Count(); i++)
                c.Pages.Insert(Pages[i]);
        }

        c.BatteryClassNames = new array<string>();
        if (BatteryClassNames)
        {
            for (i = 0; i < BatteryClassNames.Count(); i++)
                c.BatteryClassNames.Insert(BatteryClassNames[i]);
        }

        if (Limits)
            c.Limits = Limits.Copy();
        else
            c.Limits = new OZ_PdaLimits();

        c.PresetMarkers = new array<ref OZ_MapMarker>();
        if (PresetMarkers)
        {
            for (i = 0; i < PresetMarkers.Count(); i++)
            {
                if (PresetMarkers[i])
                    c.PresetMarkers.Insert(PresetMarkers[i].Copy());
            }
        }

        return c;
    }
}

class OZ_PdaVirtualDevice
{
    bool              Enabled = false;
    ref array<string> Pages;
    ref array<string> Factions;

    OZ_PdaVirtualDevice Copy()
    {
        OZ_PdaVirtualDevice c = new OZ_PdaVirtualDevice();
        c.Enabled = Enabled;

        int i;
        c.Pages = new array<string>();
        if (Pages)
        {
            for (i = 0; i < Pages.Count(); i++)
                c.Pages.Insert(Pages[i]);
        }

        c.Factions = new array<string>();
        if (Factions)
        {
            for (i = 0; i < Factions.Count(); i++)
                c.Factions.Insert(Factions[i]);
        }

        return c;
    }
}

class OZ_PdaProfilesConfig : OZ_ConfigBase
{
    ref array<ref OZ_PdaProfile> Profiles;
    ref OZ_PdaVirtualDevice      VirtualDevice;

    override int LatestVersion()
    {
        return OZ_PdaConst.SCHEMA_PROFILES;
    }

    override void LoadDefaults()
    {
        Version  = LatestVersion();
        Profiles = new array<ref OZ_PdaProfile>();

        OZ_PdaProfile p = new OZ_PdaProfile();
        p.Id          = "novice";
        p.DisplayName = "#STR_OZ_PDA_NOVICE";

        p.ClassNames = new array<string>();
        p.ClassNames.Insert("OZ_PDA_Novice");

        p.Pages = new array<string>();
        p.Pages.Insert(OZ_PdaConst.PAGE_DEVICE);
        p.Pages.Insert(OZ_PdaConst.PAGE_QUESTS);
        p.Pages.Insert(OZ_PdaConst.PAGE_CONTACTS);
        p.Pages.Insert(OZ_PdaConst.PAGE_NOTES);
        p.Pages.Insert(OZ_PdaConst.PAGE_MAP);
        p.Pages.Insert(OZ_PdaConst.PAGE_CHAT);
        p.Pages.Insert(OZ_PdaConst.PAGE_NEWS);

        p.BatteryClassNames = new array<string>();
        p.BatteryClassNames.Insert("Battery9V");

        p.Limits            = new OZ_PdaLimits();
        // Поставочні числа (ТЗ-4 R-F1.4) -- стартова точка балансу, не договір:
        // novice 25, advanced 60, sealed 10. Правляться в Profiles.json.
        p.Limits.Memory     = 25;
        Profiles.Insert(p);

        OZ_PdaProfile a = new OZ_PdaProfile();
        a.Id          = "advanced";
        a.DisplayName = "#STR_OZ_PDA_ADVANCED";

        a.ClassNames = new array<string>();
        a.ClassNames.Insert("OZ_PDA_Advanced");

        a.Pages = new array<string>();
        a.Pages.Insert(OZ_PdaConst.PAGE_DEVICE);
        a.Pages.Insert(OZ_PdaConst.PAGE_QUESTS);
        a.Pages.Insert(OZ_PdaConst.PAGE_CONTACTS);
        a.Pages.Insert(OZ_PdaConst.PAGE_NOTES);
        a.Pages.Insert(OZ_PdaConst.PAGE_MAP);
        a.Pages.Insert(OZ_PdaConst.PAGE_CHAT);
        a.Pages.Insert(OZ_PdaConst.PAGE_NEWS);

        a.BatteryClassNames = new array<string>();
        a.BatteryClassNames.Insert("Battery9V");

        a.Limits            = new OZ_PdaLimits();
        a.Limits.Memory     = 60;
        a.Limits.Friends    = 60;
        a.Limits.GroupChats = 8;
        a.ModuleSlots       = OZ_PdaConst.MODULE_SLOTS_MAX;
        a.LockAfterMinutes  = 5;
        Profiles.Insert(a);

        // Запечатаний КПК: приклад, який одразу працює, і водночас зразок
        // для моддера. Класнейм навмисно окремий -- профілі шукають за
        // класом, і «квестовий» КПК мусить бути іншим предметом, а не
        // позначкою на звичайному.
        OZ_PdaProfile q = new OZ_PdaProfile();
        q.Id          = "sealed";
        q.DisplayName = "#STR_OZ_PDA_SEALED";

        q.ClassNames = new array<string>();
        q.ClassNames.Insert("OZ_PDA_Sealed");

        q.Pages = new array<string>();
        q.Pages.Insert(OZ_PdaConst.PAGE_DEVICE);
        q.Pages.Insert(OZ_PdaConst.PAGE_MAP);
        q.Pages.Insert(OZ_PdaConst.PAGE_NOTES);

        q.BatteryClassNames = new array<string>();
        q.BatteryClassNames.Insert("Battery9V");

        q.Limits            = new OZ_PdaLimits();
        q.Limits.Memory     = 10;
        q.ModuleSlots       = 2;
        q.LockAfterMinutes  = 1;
        q.Sealed            = true;
        q.CrackSeconds      = 90;

        q.PresetMarkers = new array<ref OZ_MapMarker>();
        VirtualDevice = new OZ_PdaVirtualDevice();
        VirtualDevice.Pages    = new array<string>();
        VirtualDevice.Factions = new array<string>();
        Profiles.Insert(q);
    }

    override bool Migrate(int from)
    {
        Version = LatestVersion();
        return true;
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        if (!Profiles)
            Profiles = new array<ref OZ_PdaProfile>();

        // ВКЛАДЕНЕ -- У СТВОРЕНЕ СКРИПТОМ, І ДО ПЕРШОГО ЖЕ ВИДІЛЕННЯ.
        //
        // Лоадер кличе Validate одразу після розбору, поки читання ще чесне;
        // усе нижче в цьому методі складає рядки попереджень і заводить
        // масиви, а s_Cfg потім живе весь запуск сервера. Null лишаємо null:
        // на нього нижче є своя скарга, яка й полагодить файл.
        for (int rs = 0; rs < Profiles.Count(); rs++)
        {
            if (Profiles[rs])
                Profiles.Set(rs, Profiles[rs].Copy());
        }

        // Віртуальний термінал -- окремий вкладений об'єкт. Його два списки
        // Validate раніше не чіпав узагалі, а ворота OZ_PdaAccess ходять по
        // них на кожній перевірці.
        if (!VirtualDevice)
            VirtualDevice = new OZ_PdaVirtualDevice();
        else
            VirtualDevice = VirtualDevice.Copy();

        // NULL-ЕЛЕМЕНТ -- ЗАКОННИЙ JSON, а ForClass розіменовує кожен запис
        // на кожну операцію кожної сторінки. Викидаємо ззаду наперед і до
        // всіх інших перевірок -- та сама причина, що в Hardware.json.
        for (int pn = Profiles.Count() - 1; pn >= 0; pn--)
        {
            if (Profiles[pn])
                continue;
            OZ_Log.Warn("Profiles.json has a null entry in Profiles - dropped");
            Profiles.Remove(pn);
            warnings++;
        }

        for (int i = 0; i < Profiles.Count(); i++)
        {
            OZ_PdaProfile p = Profiles[i];

            if (!p.ClassNames || p.ClassNames.Count() == 0)
            {
                OZ_Log.Warn("profile \"" + p.Id + "\" has no ClassNames - nothing will ever match it");
                warnings++;
            }

            for (int c = 0; p.ClassNames && c < p.ClassNames.Count(); c++)
            {
                // Лише існування в конфізі гри. Спорідненість не перевіряємо
                // навмисно -- див. коментар угорі файлу.
                if (!GetGame().ConfigIsExisting("CfgVehicles " + p.ClassNames[c]))
                {
                    string wc = "profile \"" + p.Id;
                    wc += "\": class \"" + p.ClassNames[c];
                    wc += "\" is not in CfgVehicles";
                    OZ_Log.Warn(wc);
                    warnings++;
                }
            }

            if (!p.Pages)
                p.Pages = new array<string>();

            for (int g = 0; g < p.Pages.Count(); g++)
            {
                if (!OZ_PageRegistry.Has(p.Pages[g]))
                {
                    string wp = "profile \"" + p.Id;
                    wp += "\": page \"" + p.Pages[g];
                    wp += "\" is not registered";
                    OZ_Log.Warn(wp);
                    warnings++;
                }
            }

            if (!p.Limits)
                p.Limits = new OZ_PdaLimits();

            // Limits.Memory -- ЄДИНЕ джерело числа ячейок (ТЗ-4 R-F1.2). Немає
            // -- WARNING з іменем профілю й задокументоване умовчання, а не
            // мовчазний перехід на константу з іншого файлу (R-F1.3).
            if (p.Limits.Memory <= 0)
            {
                string wmem = "profile \"" + p.Id + "\" declares no Limits.Memory - using ";
                wmem += OZ_PdaConst.MEMORY_DEFAULT.ToString() + " cells; set it in Profiles.json";
                OZ_Log.Warn(wmem);
                p.Limits.Memory = OZ_PdaConst.MEMORY_DEFAULT;
                warnings++;
            }

            // Friends -- НЕ той випадок, де нуль можна лишити як прочитався.
            // Після Copy() ключ, якого у файлі немає, і чесний нуль у файлі
            // невідрізненні, а читає їх OZ_PdaContactSwap.Full() як «стелі
            // немає». З двох прочитань забутого ключа безпечне саме
            // задокументоване число: забутий ключ, що МОВЧКИ ЗНІМАЄ межу,
            // гірший за адміна, який втратив нуль як спосіб її зняти. Кому
            // потрібен записник без стелі -- пише велике число, і воно видно
            // у файлі, а не вгадується з відсутності рядка.
            //
            // Мовчки й без warnings++: інакше кожен профіль без цього ключа
            // переписував би файл на завантаженні (OZ_ConfigBase.Validate ->
            // write). Той самий хід, що в OZF_Settings.InviteTtlSeconds.
            if (p.Limits.Friends <= 0)
                p.Limits.Friends = OZ_PdaConst.FRIENDS_DEFAULT;
            if (!p.BatteryClassNames)
                p.BatteryClassNames = new array<string>();
            if (!p.PresetMarkers)
                p.PresetMarkers = new array<ref OZ_MapMarker>();

            // NULL-МІТКА В ПРЕСЕТІ -- законний JSON, і посів її розіменовує
            // (OZ_PDA_Base.OZ_SeedFromProfile). Викидаємо тут, щоб предмет
            // не помирав під час появи у світі.
            for (int pm = p.PresetMarkers.Count() - 1; pm >= 0; pm--)
            {
                if (p.PresetMarkers[pm])
                    continue;
                OZ_Log.Warn("profile \"" + p.Id + "\" has a null entry in PresetMarkers - dropped");
                p.PresetMarkers.Remove(pm);
                warnings++;
            }

            if (p.CrackSeconds < 0)
            {
                OZ_Log.Warn("profile \"" + p.Id + "\" has a negative CrackSeconds, clamped to 0");
                p.CrackSeconds = 0;
                warnings++;
            }

            if (p.ModuleSlots < 0 || p.ModuleSlots > OZ_PdaConst.MODULE_SLOTS_MAX)
            {
                string wm = "profile \"" + p.Id;
                wm += "\" asks for " + p.ModuleSlots.ToString();
                wm += " module bays; the config declares at most ";
                wm += OZ_PdaConst.MODULE_SLOTS_MAX.ToString();
                OZ_Log.Warn(wm);
                p.ModuleSlots = Math.Clamp(p.ModuleSlots, 0, OZ_PdaConst.MODULE_SLOTS_MAX);
                warnings++;
            }

            if (p.LockAfterMinutes < 0)
            {
                OZ_Log.Warn("profile \"" + p.Id + "\" has a negative LockAfterMinutes, clamped to 0");
                p.LockAfterMinutes = 0;
                warnings++;
            }
        }
    }
}

class OZ_PdaProfiles
{
    private static ref OZ_PdaProfilesConfig s_Cfg;

    static OZ_PdaProfilesConfig Get()
    {
        return s_Cfg;
    }

    static int Count()
    {
        if (!s_Cfg)
            return 0;
        return s_Cfg.Profiles.Count();
    }

    static void ServerLoad()
    {
        s_Cfg = new OZ_PdaProfilesConfig();
        OZ_ConfigLoader<OZ_PdaProfilesConfig>.Load(OZ_PdaConst.PROFILES, "Profiles", s_Cfg);
    }

    // Класи, про які ми вже поскаржились. Скарга потрібна ОДНА на клас за
    // сеанс: цю функцію питають на кожну операцію кожної сторінки, і без
    // пам'яті вона залила б лог тим самим рядком.
    private static ref array<string> s_Unknown;

    static OZ_PdaProfile ForClass(string cls)
    {
        if (!s_Cfg)
            return null;

        for (int i = 0; i < s_Cfg.Profiles.Count(); i++)
        {
            if (!s_Cfg.Profiles[i])
                continue;
            array<string> names = s_Cfg.Profiles[i].ClassNames;
            for (int c = 0; names && c < names.Count(); c++)
            {
                if (names[c] == cls)
                    return s_Cfg.Profiles[i];
            }
        }

        // КЛАС НАЗИВАЄТЬСЯ ВГОЛОС.
        //
        // Прилад без запису в OZ_PDA_Profiles.json не робить нічого: сторінок
        // немає, стелі немає, живлення немає. Мовчазний null лишав адміна з
        // приладом, який просто «не працює», і жодної підказки, що виправити
        // треба ім'я класу в конфізі.
        if (!s_Unknown)
            s_Unknown = new array<string>();

        if (s_Unknown.Find(cls) == -1)
        {
            s_Unknown.Insert(cls);
            OZ_Log.Warn("pda profile: class \"" + cls + "\" is in no profile of OZ_PDA_Profiles.json - the device will do nothing");
        }

        return null;
    }
}
