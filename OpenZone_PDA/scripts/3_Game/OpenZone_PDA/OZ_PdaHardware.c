// Залізо, яке вставляється в КПК.
//
// Три різні речі, а не один список слотів:
//
//   ЖИВЛЕННЯ   -- батарея. Слот ванільний, рушій сам втикає в неї пристрій.
//   СХОВИЩЕ    -- носій даних. Свій слот: це вміст, а не здатність.
//   МОДУЛІ     -- антена, радіометр, дозиметр і що завгодно від інших модів.
//                 Ділять ОБМЕЖЕНЕ число відсіків.
//
// Відсіків обмежено навмисно: це головний важіль тиру. У ПДА новачка один, у
// долговського три -- і гравець сам вирішує, що нести: далекий зв'язок чи
// лічильник Гейгера. Список слотів без обмеження такого вибору не дає.
//
// Що саме влазить у відсік і що це дає -- таблиця класнеймів нижче. Пізнаємо
// ЗВІРКОЮ КЛАСНЕЙМА, ніколи спорідненістю: тоді модуль може принести будь-який
// мод, а адмін -- навести КПК на предмет, про який ми не чули.

class OZ_ModuleSpec
{
    string ClassName   = "";
    string DisplayName = "";

    // ХТО ВОЛОДІЄ ЗАПИСОМ (D94, друга половина). Порожньо -- запис адміна:
    // файл головніший, чуже оголошення того самого класу його не чіпає.
    // Непорожньо -- запис приніс інший мод (Declare ставить назву мода або
    // "mod"): він потрапляє у файл, щоб адмін його БАЧИВ у редакторі, але
    // належить модові -- наступне оголошення того ж класу його замінює.
    // Хочеш перекрити чужий модуль назавжди -- зітри Origin: запис стає
    // твоїм, і файл знову виграє.
    string Origin = "";

    // Що це за прилад. Рядком, а не числом: мод-постачальник не мусить знати
    // наших констант, а адмін бачить у JSON слово, а не код.
    //
    //   "antenna"     -- вмикає далекий зв'язок, дає радіус
    //   "radiometer"  -- лічильник Гейгера: зовнішнє поле тут і зараз
    //   "dosimeter"   -- ін'єкційний: накопичена доза в тілі
    //   будь-що інше  -- чужий модуль, КПК просто вмикає його сторінки
    string Kind = "";

    // ШПИГУНСЬКИЙ транспондер: скільки хвилин активної роботи в платі.
    // Нуль -- звичайний модуль. Вичерпаний ресурс СПАЛЮЄ плату, як
    // дешифратор: одноразова розкіш, не вічне око.
    float SpyMinutes = 0;

    // Радіус упевненого прийому в метрах. Має сенс лише для "antenna".
    // Нуль означає «покриття задає щось інше» -- наприклад стаціонарна вежа.
    float RangeM = 0;

    // Адмінська позначка того, наскільки модуль мав би піднімати витрату
    // живлення понад базову. Сьогодні лише дані: жодне місце коду не множить
    // це число на energyUsagePerSecond (config.cpp), тож зміна поля в
    // Hardware.json чи у формі VPP витрату не змінює (знахідка 98, задача
    // 52) -- ефекту "радіометр їсть більше" в грі немає.
    float PowerFactor = 1.0;

    // Які сторінки модуль вмикає. Антена вмикає "radio", радіометр -- свою
    // шкалу; поле загальне, щоб чужий модуль міг увімкнути свою сторінку.
    ref array<string> EnablesPages;
}

class OZ_CarrierSpec
{
    string ClassName   = "";
    string DisplayName = "";
    // Чи можна перезаписати носій із КПК. Одноразовий чип із чужої схованки
    // перезаписувати не можна -- у цьому половина його цінності.
    bool   Writable    = true;
    // Місткість -- ОДНЕ число, в записах, байдуже яких. Мітка -- запис,
    // нотатка -- запис, точка маршруту -- запис, частота -- запис. 0 -- без
    // стелі.
    //
    // Раніше стелі були окремі на кожен відомий рід, і саме це робило носій
    // закритим: щоб мод поклав на нього своє, він мусив би прописати собі
    // стелю в КОЖНОМУ класі носія, а автор носія -- знати всі майбутні роди.
    // Спільний лічильник знімає обидві половини цієї вимоги.
    //
    // Другий важіль тиру після відсіків: дискета на шість записів і польовий
    // накопичувач -- різні речі за той самий слот.
    int    MaxRecords  = 0;
}

class OZ_PdaHardwareConfig : OZ_ConfigBase
{
    ref array<ref OZ_ModuleSpec>  Modules;
    ref array<ref OZ_CarrierSpec> Carriers;

    override int LatestVersion()
    {
        return 1;
    }

    override void LoadDefaults()
    {
        Version  = LatestVersion();
        Modules  = new array<ref OZ_ModuleSpec>();
        Carriers = new array<ref OZ_CarrierSpec>();

        OZ_ModuleSpec radio = new OZ_ModuleSpec();
        radio.ClassName    = "OZ_Module_Radiometer";
        radio.DisplayName  = "#STR_OZ_MOD_RADIOMETER";
        radio.Kind         = "radiometer";
        radio.PowerFactor  = 1.4;
        radio.EnablesPages = new array<string>();
        Modules.Insert(radio);

        OZ_ModuleSpec dose = new OZ_ModuleSpec();
        dose.ClassName    = "OZ_Module_Dosimeter";
        dose.DisplayName  = "#STR_OZ_MOD_DOSIMETER";
        dose.Kind         = "dosimeter";
        // Ін'єкційний датчик живиться сам і батарею КПК майже не чіпає.
        dose.PowerFactor  = 1.05;
        dose.EnablesPages = new array<string>();
        Modules.Insert(dose);

        // Базова антена ЙДЕ З КПК, а не з мода рації. Транспондер -- функція
        // самого пристрою, і ставити його в залежність від ще не написаного
        // мода означало б віддати КПК без того, заради чого його носять.
        //
        // Далекі антени й вежі приносить OpenZone Radio; вони просто мають
        // більший RangeM і перекривають цю.
        OZ_ModuleSpec ant = new OZ_ModuleSpec();
        ant.ClassName    = "OZ_Module_Antenna";
        ant.DisplayName  = "#STR_OZ_MOD_ANTENNA";
        ant.Kind         = "antenna";
        ant.RangeM       = 500;
        // Передавач їсть більше за будь-який датчик, і це має бути видно по
        // батареї.
        ant.PowerFactor  = 1.6;
        ant.EnablesPages = new array<string>();
        Modules.Insert(ant);

        // GPS. Без нього прилад НЕ ЗНАЄ, де він (ТЗ-4 R-B2.2): мітки й маршрут
        // працюють, а «ти тут» і відстані -- ні. Окремий модуль через той самий
        // договір, що й решта (R-B2.3), а не окремий механізм.
        OZ_ModuleSpec gps = new OZ_ModuleSpec();
        gps.ClassName    = "OZ_Module_GPS";
        gps.DisplayName  = "#STR_OZ_MOD_GPS";
        gps.Kind         = "gps";
        gps.PowerFactor  = 1.2;
        gps.EnablesPages = new array<string>();
        Modules.Insert(gps);

        // Шпигунська антена ЗАПИСАНА в залізі (ТЗ-4 R-F4.1): предмет спавнився,
        // а запису не мав -- і з коробки не робив нічого. Антена як антена,
        // плюс лічені хвилини ока на всіх (SpyMinutes), потім плата згорає.
        OZ_ModuleSpec spy = new OZ_ModuleSpec();
        spy.ClassName    = "OZ_Module_SpyAntenna";
        spy.DisplayName  = "#STR_OZ_MOD_SPY";
        spy.Kind         = "antenna";
        spy.RangeM       = 500;
        spy.SpyMinutes   = 60;
        spy.PowerFactor  = 2.0;
        spy.EnablesPages = new array<string>();
        Modules.Insert(spy);

        // Дешифратор. Відкриває запечатані КПК -- і тільки їх; на звичайному
        // пристрої він просто займає відсік. Їсть багато: він рахує.
        OZ_ModuleSpec dec = new OZ_ModuleSpec();
        dec.ClassName    = "OZ_Module_Decryptor";
        dec.DisplayName  = "#STR_OZ_MOD_DECRYPTOR";
        dec.Kind         = "decryptor";
        dec.PowerFactor  = 2.0;
        dec.EnablesPages = new array<string>();
        Modules.Insert(dec);

        // Три класи -- три місткості. Числа СТЕНДОВІ: баланс задає адмін
        // у Hardware.json, а не цей файл.
        OZ_CarrierSpec floppy = new OZ_CarrierSpec();
        floppy.ClassName   = "OZ_DataCarrier_Floppy";
        floppy.DisplayName = "#STR_OZ_CARRIER_FLOPPY";
        floppy.Writable    = true;
        floppy.MaxRecords  = 6;
        Carriers.Insert(floppy);

        OZ_CarrierSpec chip = new OZ_CarrierSpec();
        chip.ClassName   = "OZ_DataCarrier_Chip";
        chip.DisplayName = "#STR_OZ_CARRIER_CHIP";
        chip.Writable    = true;
        chip.MaxRecords  = 24;
        Carriers.Insert(chip);

        OZ_CarrierSpec drive = new OZ_CarrierSpec();
        drive.ClassName   = "OZ_DataCarrier_Drive";
        drive.DisplayName = "#STR_OZ_CARRIER_DRIVE";
        drive.Writable    = true;
        Carriers.Insert(drive);
    }

    override bool Migrate(int from)
    {
        Version = LatestVersion();
        return true;
    }

    // Індекси-дублікати в переліку класнеймів, ЗЗАДУ НАПЕРЕД -- саме в тому
    // порядку, у якому їх можна видаляти, не зсуваючи решту.
    //
    // Одна петля на обидва масиви: правило однакове, а два її списки жили
    // поруч і розходилися б від першої ж правки в одному з них.
    private void DupsBack(array<string> names, array<int> outDrop)
    {
        for (int d = names.Count() - 1; d >= 0; d--)
        {
            for (int f = 0; f < d; f++)
            {
                if (names[f] == names[d])
                {
                    outDrop.Insert(d);
                    break;
                }
            }
        }
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        if (!Modules)
            Modules = new array<ref OZ_ModuleSpec>();
        if (!Carriers)
            Carriers = new array<ref OZ_CarrierSpec>();

        // NULL-ЕЛЕМЕНТ -- ЦІЛКОМ ЗАКОННИЙ JSON, і саме він валив усе далі.
        //
        // `"Modules": [null]` розбирається без помилки, а перша ж перевірка
        // нижче розіменовує його -- і сервер отримує виняток не на буті, а на
        // кожному запиті, який спитає ModuleFor. Викидаємо ззаду наперед, до
        // будь-якої іншої перевірки: після цього ніхто в цьому файлі не мусить
        // питати про null.
        for (int mn = Modules.Count() - 1; mn >= 0; mn--)
        {
            if (Modules[mn])
                continue;
            OZ_Log.Warn("Hardware.json has a null entry in Modules - dropped");
            Modules.Remove(mn);
            warnings++;
        }
        for (int cn = Carriers.Count() - 1; cn >= 0; cn--)
        {
            if (Carriers[cn])
                continue;
            OZ_Log.Warn("Hardware.json has a null entry in Carriers - dropped");
            Carriers.Remove(cn);
            warnings++;
        }

        // ДУБЛІКАТ КЛАСНЕЙМА ВІДКИДАЄМО, і кажемо про це вголос.
        //
        // Пошук завжди повертає ПЕРШИЙ запис, тож другий не працює ніколи --
        // а форма в адмінці показує його як збережений. Адмін правив другий,
        // бачив його і в списку, і у файлі, і не розумів, чому прилад
        // поводиться по-старому. Лишаємо саме перший -- той, який і працює.
        array<string> mnames = new array<string>();
        for (int mi = 0; mi < Modules.Count(); mi++)
            mnames.Insert(Modules[mi].ClassName);

        array<int> mdrop = new array<int>();
        DupsBack(mnames, mdrop);
        for (int md = 0; md < mdrop.Count(); md++)
        {
            OZ_Log.Warn("module \"" + mnames[mdrop[md]] + "\" is declared twice in Hardware.json - only the first entry ever worked, the later one is dropped");
            Modules.Remove(mdrop[md]);
            warnings++;
        }

        for (int i = 0; i < Modules.Count(); i++)
        {
            OZ_ModuleSpec m = Modules[i];

            if (!GetGame().ConfigIsExisting("CfgVehicles " + m.ClassName))
            {
                OZ_Log.Warn("module class \"" + m.ClassName + "\" is not in CfgVehicles - is its mod loaded?");
                warnings++;
            }

            if (!m.EnablesPages)
                m.EnablesPages = new array<string>();

            if (m.Kind == "")
            {
                OZ_Log.Warn("module \"" + m.ClassName + "\" has no Kind - it will attach but do nothing");
                warnings++;
            }

            if (m.RangeM < 0)
            {
                OZ_Log.Warn("module \"" + m.ClassName + "\" has a negative RangeM, clamped to 0");
                m.RangeM = 0;
                warnings++;
            }

            if (m.PowerFactor < 0)
            {
                OZ_Log.Warn("module \"" + m.ClassName + "\" has a negative PowerFactor, clamped to 1");
                m.PowerFactor = 1.0;
                warnings++;
            }
        }

        // Носії -- те саме правило й та сама петля: пошук бере перший, отже
        // другий мертвий.
        array<string> cnames = new array<string>();
        for (int ci = 0; ci < Carriers.Count(); ci++)
            cnames.Insert(Carriers[ci].ClassName);

        array<int> cdrop = new array<int>();
        DupsBack(cnames, cdrop);
        for (int cx = 0; cx < cdrop.Count(); cx++)
        {
            OZ_Log.Warn("carrier \"" + cnames[cdrop[cx]] + "\" is declared twice in Hardware.json - only the first entry ever worked, the later one is dropped");
            Carriers.Remove(cdrop[cx]);
            warnings++;
        }

        for (int c = 0; c < Carriers.Count(); c++)
        {
            if (!GetGame().ConfigIsExisting("CfgVehicles " + Carriers[c].ClassName))
            {
                OZ_Log.Warn("carrier class \"" + Carriers[c].ClassName + "\" is not in CfgVehicles - is its mod loaded?");
                warnings++;
            }
        }
    }
}

class OZ_PdaHardware
{
    private static ref OZ_PdaHardwareConfig s_Cfg;

    // ЧИ МОЖНА ПИСАТИ У ФАЙЛ. Load() відповідає false, коли на диску лежить
    // єдиний примірник, якого він не зрозумів і не зміг винести в карантин.
    // Досі відповідь ігнорувалась, і найближче чуже оголошення переписувало
    // той файл дефолтами плюс свій модуль -- разом із усім, що адмін туди
    // вписав. Копія ядра цього правила -- OZ_Settings.Writable().
    private static bool s_Writable = true;

    // Черга чужих оголошень.
    //
    // ПОРЯДОК МОДУЛІВ CF НЕ ГАРАНТОВАНИЙ. Виміряно на стенді: мод рації
    // отримав OnMissionStart РАНІШЕ за КПК, конфіг заліза ще не був
    // завантажений, і всі його оголошення пішли в нікуди -- тихо, з
    // «modules=0» у власному ж рядку готовності.
    //
    // Тому Declare нічого не вимагає від порядку: якщо конфіга ще немає,
    // оголошення чекає в черзі, а ServerLoad його забирає.
    //
    // ЧЕРГА НЕ ЧИСТИТЬСЯ ПІСЛЯ ПЕРШОГО ЗАВАНТАЖЕННЯ, і це головне в ній.
    //
    // Раніше вона обнулялась, і це ламало гаряче застосування заліза з
    // вкладки VPP: адмін тиснув SAVE, ServerLoad перечитував файл із диска
    // -- а чужі оголошення жили ЛИШЕ в пам'яті, і після перечитування їх
    // не було. Плата рації переставала впізнаватись до найближчого
    // рестарту, і жодного рядка про це ніде.
    //
    // Тепер список -- це пам'ять про все, що оголосили чужі моди за цей
    // запуск, і ServerLoad накладає його поверх файла КОЖНОГО разу. Адмін
    // лишається головнішим: Insert не чіпає клас, який уже є у файлі.
    private static ref array<ref OZ_ModuleSpec> s_Declared;

    static OZ_PdaHardwareConfig Get()      { return s_Cfg; }

    static int ModuleCount()
    {
        if (!s_Cfg)
            return 0;
        return s_Cfg.Modules.Count();
    }

    static int CarrierCount()
    {
        if (!s_Cfg)
            return 0;
        return s_Cfg.Carriers.Count();
    }

    static void ServerLoad()
    {
        s_Cfg = new OZ_PdaHardwareConfig();
        s_Writable = OZ_ConfigLoader<OZ_PdaHardwareConfig>.Load(OZ_PdaConst.HARDWARE, "Hardware", s_Cfg);

        // Чужі оголошення накладаємо ПІСЛЯ завантаження конфіга -- адмін
        // лишається головнішим -- і робимо це при КОЖНОМУ завантаженні, а не
        // лише при першому: гаряче застосування з вкладки VPP теж проходить
        // сюди.
        bool changed = false;
        for (int i = 0; s_Declared && i < s_Declared.Count(); i++)
        {
            if (Insert(s_Declared[i]))
                changed = true;
        }

        // Чужі записи -- У ФАЙЛ (D94, друга половина). Досі вони жили лише в
        // пам'яті, і редактор RAW JSON у VPP їх не показував: адмін не міг ні
        // побачити плату рації, ні перекрити її, не знаючи класнейма. Тепер
        // після кожного завантаження файл наздоганяє пам'ять; що кому
        // належить, каже Origin. Пишемо лише коли щось справді додалось.
        if (changed && s_Writable)
        {
            OZ_ConfigLoader<OZ_PdaHardwareConfig>.Save(OZ_PdaConst.HARDWARE, "Hardware", s_Cfg);
            OZ_Log.Info("hardware: foreign module declarations written to Hardware.json");
        }
        else if (changed)
        {
            OZ_Log.Warn("hardware: Hardware.json is not writable, foreign module declarations stay in memory only");
        }
    }

    // Чуже залізо. Мод, що приносить свій модуль, оголошує його ОДНИМ рядком
    // зі свого OnMissionStart -- після ServerLoad КПК:
    //
    //     OZ_PdaHardware.Declare(spec);
    //
    // АДМІН ГОЛОВНІШИЙ. Якщо в Hardware.json уже є запис із таким класнеймом,
    // ми його не чіпаємо: власник сервера мусить мати змогу перенастроїти
    // чужий модуль, не правлячи чужий мод. Тому це «оголосити, якщо ще нема»,
    // а не «записати».
    static bool Declare(OZ_ModuleSpec spec)
    {
        if (!spec || spec.ClassName == "")
            return false;

        if (!spec.EnablesPages)
            spec.EnablesPages = new array<string>();
        if (spec.Origin == "")
            spec.Origin = "mod";

        // ЗАПАМ'ЯТОВУЄМО ЗАВЖДИ -- і коли конфіг уже є, і коли ще ні.
        //
        // Список потрібен не лише для черги: він переживає перечитування
        // конфіга (гаряче застосування з VPP) і накладається знову.
        if (!s_Declared)
            s_Declared = new array<ref OZ_ModuleSpec>();
        if (!Known(spec.ClassName))
            s_Declared.Insert(spec);

        // Конфіга ще немає -- лишаємось у черзі. Відповідаємо true:
        // оголошення ПРИЙНЯТО, і мод, який його зробив, має право так вважати.
        if (!s_Cfg)
            return true;

        // Конфіг уже є -- пишемо у файл одразу. Порядок OnMissionStart між
        // модами не гарантований (див. чергу вище): коли КПК завантажився
        // ПЕРШИМ, оголошення не проходить через ServerLoad, і без цього рядка
        // файл наздогнав би пам'ять лише наступним перечитуванням.
        bool added = Insert(spec);
        if (added && s_Writable)
        {
            OZ_ConfigLoader<OZ_PdaHardwareConfig>.Save(OZ_PdaConst.HARDWARE, "Hardware", s_Cfg);
            OZ_Log.Info("hardware: a module declared after the load is written to Hardware.json: " + spec.ClassName);
        }
        return added;
    }

    private static bool Known(string cls)
    {
        for (int i = 0; s_Declared && i < s_Declared.Count(); i++)
        {
            if (s_Declared[i] && s_Declared[i].ClassName == cls)
                return true;
        }
        return false;
    }

    private static bool Insert(OZ_ModuleSpec spec)
    {
        if (!s_Cfg || !spec)
            return false;

        OZ_ModuleSpec had = ModuleFor(spec.ClassName);
        if (had)
        {
            // Запис адміна (Origin порожній) -- головніший, не чіпаємо.
            if (had.Origin == "")
                return false;

            // Запис мода -- мод і оновлює. Той самий вміст переписувати нема
            // чого; це й тримає файл у спокої між запусками.
            if (Same(had, spec))
                return false;

            int at = s_Cfg.Modules.Find(had);
            if (at >= 0)
                s_Cfg.Modules.Set(at, spec);
            OZ_Log.Dbg("module re-declared by another mod: " + spec.ClassName);
            return true;
        }

        s_Cfg.Modules.Insert(spec);
        OZ_Log.Dbg("module declared by another mod: " + spec.ClassName);
        return true;
    }

    private static bool Same(OZ_ModuleSpec a, OZ_ModuleSpec b)
    {
        if (a.DisplayName != b.DisplayName || a.Kind != b.Kind || a.Origin != b.Origin)
            return false;
        if (a.SpyMinutes != b.SpyMinutes || a.RangeM != b.RangeM || a.PowerFactor != b.PowerFactor)
            return false;
        if (!a.EnablesPages || !b.EnablesPages || a.EnablesPages.Count() != b.EnablesPages.Count())
            return false;
        for (int i = 0; i < a.EnablesPages.Count(); i++)
        {
            if (a.EnablesPages[i] != b.EnablesPages[i])
                return false;
        }
        return true;
    }

    static OZ_ModuleSpec ModuleFor(string cls)
    {
        if (!s_Cfg)
            return null;

        // null-guard: Validate вичищає порожні записи, але Declare теж пише
        // в цей масив, і одне місце, де правило порушать, не має валити
        // кожен запит статусу.
        for (int i = 0; i < s_Cfg.Modules.Count(); i++)
        {
            if (s_Cfg.Modules[i] && s_Cfg.Modules[i].ClassName == cls)
                return s_Cfg.Modules[i];
        }
        return null;
    }

    static OZ_CarrierSpec CarrierFor(string cls)
    {
        if (!s_Cfg)
            return null;

        for (int i = 0; i < s_Cfg.Carriers.Count(); i++)
        {
            if (s_Cfg.Carriers[i] && s_Cfg.Carriers[i].ClassName == cls)
                return s_Cfg.Carriers[i];
        }
        return null;
    }
}
