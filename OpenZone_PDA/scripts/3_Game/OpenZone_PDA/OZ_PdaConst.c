class OZ_PdaConst
{
    static const string PROFILES = "$profile:OpenZone\\OZ_PDA_Profiles.json";

    static const string HARDWARE = "$profile:OpenZone\\OZ_PDA_Hardware.json";

    static const int SCHEMA_PROFILES = 1;

    // Імена слотів. Батарея -- ванільна: рушій сам втикає пристрій у неї.
    // Решта наші, і рушій про них не знає нічого -- що в них влазить і що це
    // дає, вирішує таблиця класнеймів у Hardware.json.
    static const string SLOT_BATTERY = "BatteryD";
    static const string SLOT_CARRIER = "OZ_DataCarrier";
    // Слот носіння на САМОМУ ГРАВЦЕВІ. Пристрій активний лише в руках або
    // тут: у рюкзаку він мовчить -- і за задумом, і тому, що так не треба
    // обходити інвентар на кожен запит.
    static const string SLOT_WEAR    = "OZ_PdaWear";

    // Модульні відсіки. Оголошені ВСІ, бо слоти не додаються в рантаймі;
    // скільки з них видно на конкретній моделі -- вирішує профіль, а зайві
    // ховає CanDisplayAttachmentSlot.
    static const int    MODULE_SLOTS_MAX = 3;
    static const string SLOT_MODULE_1 = "OZ_Module1";
    static const string SLOT_MODULE_2 = "OZ_Module2";
    static const string SLOT_MODULE_3 = "OZ_Module3";

    // ЛАДЕР, а не "OZ_Module" + (i+1): ця функція стоїть у обході відсіків,
    // а склейка рядка -- алокація на кожен виток. Два рядки економії коштували
    // б більше, ніж економлять.
    static string ModuleSlot(int i)
    {
        if (i == 0) return SLOT_MODULE_1;
        if (i == 1) return SLOT_MODULE_2;
        if (i == 2) return SLOT_MODULE_3;
        return "";
    }

    // Види модулів. Рядками, бо ці ж слова стоять у JSON і їх читає адмін.
    static const string MOD_ANTENNA    = "antenna";
    static const string MOD_RADIOMETER = "radiometer";
    static const string MOD_DECRYPTOR  = "decryptor";
    static const string MOD_DOSIMETER  = "dosimeter";
    // Приймач GPS (ТЗ-4 R-B2.2): без нього прилад не знає, де він.
    static const string MOD_GPS        = "gps";

    // Id меню для EnterScriptedMenu.
    //
    // Мусить бути > 46 (останній ванільний у constants.c), але й НЕ завеликим:
    // рушій відкидає завеликий id ще ДО того, як спитати місію -- метод
    // EnterScriptedMenu просто повертає NULL, а Mission.CreateScriptedMenu
    // навіть не викликається. Сусідній мод спіймав це діагностикою: у лозі є
    // виклики з ванільними 11 і 17, а з шестизначним -- жодного.
    //
    // Реєстру id у рушії немає, тож зіткнення з ЧУЖИМ модом можливе й
    // непереборне. Єдиний захист -- перевіряти FindMenu перед відкриттям.
    static const int MENU_PDA = 131;
    // Редактор розкладки HUD -- сусіднє меню з тими самими застереженнями.
    static const int MENU_PDA_HUD = 132;

    // Тут лежало INPUT_OPEN = "UAOZPdaOpen" -- ім'я інпута з data/inputs.xml.
    // Рішення власника 2026-09-08: клавіші відкриття немає. Прилад відкриває
    // дія на предметі в руках, і більш ніщо; сам inputs.xml прибрано.

    // Сторінки, які несе сам КПК.
    //
    // PAGE_QUESTS -- договір, а не вміст: КПК малює журнал, а завдання в нього
    // кладе квестовий мод через OZ_PdaQuests.Bind(). Так журнал лишається
    // один, чий би мод його не наповнював.
    static const string PAGE_DEVICE = "device";

    // Тут лежало OP_VIRTUAL_OPEN = "virtual_open" -- вхід у «КПК без
    // предмета» (D132), єдина операція, яку клієнт слав із порожніми руками.
    // Рішення власника 2026-09-08: порожні руки не шлють нічого.

    // Ключі в пакеті синхронізації ядра (OZ_SyncExtras, D87): два числа з
    // OZ_PDA_Tuning.json, які потрібні худу КОЖНОГО гравця, а не лише того,
    // хто носить антену.
    static const string SYNC_TOAST_S = "pda.toast_s";
    static const string SYNC_ROUTE_M = "pda.route_m";
    // Стеля повідомлення -- клієнтові, щоб лічильник рахував за тим самим
    // числом, що й сервер (ТЗ-4 R-D1.3).
    static const string SYNC_MSG_MAX = "pda.msg_max";
    static const string PAGE_QUESTS = "quests";
    static const string PAGE_CONTACTS = "contacts";
    static const string PAGE_NOTES    = "notes";
    static const string PAGE_MAP      = "map";
    static const string PAGE_NEWS     = "news";
    static const string PAGE_CHAT     = "chat";

    // Задокументоване УМОВЧАННЯ ячейок пам'яті для профілю, який їх не
    // оголосив (ТЗ-4 R-F1.3): застосовується з WARNING на ім'я профілю, а не
    // мовчки. Єдине джерело числа для приладу -- Limits.Memory у Profiles.json.
    static const int MEMORY_DEFAULT = 25;

    // Задокументоване УМОВЧАННЯ місткості записника. Те саме число, що стоїть
    // ініціалізатором у OZ_PdaLimits.Friends, і живе воно тут, бо Validate
    // мусить його виставити: після Copy() ключ, якого у файлі немає, читається
    // нулем, а нуль OZ_PdaContactSwap.Full() розуміє як «стелі немає» -- тобто
    // забутий ключ не звужував би межу, а СКАСОВУВАВ її.
    static const int FRIENDS_DEFAULT = 20;

    // Те саме для групових бесід (ТЗ-4 R-F1.6). Нуль їде до моста полем
    // OZ_ChatAskGroup.Max, а там він означає «без межі», -- тобто пастка
    // рівно та сама, що у Friends, і лікується так само. Число 2 --
    // поставочне значення профілю novice.
    static const int GROUPCHATS_DEFAULT = 2;

    // Скільки одиниць енергії за хвилину їсть САМ прилад, без плат
    // (ТЗ-5 R-B2.2). Помножене на добуток PowerFactor вставлених плат, це
    // й є витрата, яку OZ_PDA_Base віддає рушійному лічильнику. 0.5 --
    // те саме число, що стояло плоским energyUsagePerSecond = 0.0083 у
    // config.cpp (0.0083 x 60 = 0.498).
    static const float DRAIN_DEFAULT = 0.5;

    // Довжина ПІН-коду для профілю, який її не оголосив (ТЗ-5 R-B3.3).
    static const int PIN_LENGTH_DEFAULT = 4;
    // Межі, у яких профіль може її задати. Знизу -- чотири (менше не код,
    // а підказка), зверху -- вісім: пад малює рівно стільки крапок,
    // скільки цифр, і рядок із них мусить лишитись у своєму віджеті.
    static const int PIN_LENGTH_MIN = 4;
    static const int PIN_LENGTH_MAX = 8;

    // Стеля ІМЕНІ ПЕРСОНИ на дорозі до моста (ТЗ-6 R1.2/R2.1). Стала й
    // навмисно не налаштовується: міст звіряє це ім'я ТОЧНО, тож будь-яка
    // адмінська ручка, опущена нижче за довжину персони, робила б підпис
    // несумісним із власним списком голосів. Число щедре -- воно тут лише
    // проти підробленого RPC, а не як межа для людини.
    //
    // Сама персона -- ВІЛЬНИЙ РЯДОК без межі з боку моста (карбується
    // /openzone persona create, лежить у personas.json дослівно), і право
    // на неї роздає МІСТ (ТЗ-6 R2.3). Тобто це не «довшого імені не буває»,
    // а «довше за це -- вже не ім'я».
    static const int PERSONA_MAX_BYTES = 190;

    // РЕШТА МЕЖ ТУТ БІЛЬШЕ НЕ ЖИВЕ (ревізія 2026-09-06). Одинадцять констант
    // -- довжини назв, тіл, історії, складу групи -- були ДРУГИМ комплектом
    // поставочних чисел поруч із полями OZ_PdaTuning, і жоден із них не
    // звірявся з іншим. Тепер число одне, і воно в Tuning.json.
    //
    // CHAT_MSG_MAX лишився: його читає КЛІЄНТ (OZ_PdaPageChat.MsgMax) як
    // запасне, поки пакет синхронізації ядра не привіз pda.msg_max, а
    // OZ_PdaTuning на клієнті стоїть на своїх поставочних і про адмінське
    // число не знає.
    static const int CHAT_MSG_MAX   = 1000;

    // Наскільки близько треба клікнути, щоб влучити в наявну мітку, у метрах.
    // Не в пікселях: на різних масштабах піксель означає різну відстань, і
    // «влучив» мало б залежати від зуму.
    static const float MARKER_PICK_M = 40;

    // КОЛЬОРИ МІТОК НА КАРТІ -- одним місцем на дві карти.
    //
    // Це НЕ токени палітри: у ui/tokens.json їх немає й бути не мусить --
    // палітра описує поверхні й текст, а це семантика мітки. Але число тут
    // одне на мінікарту й на велику: вони малюють ті самі мітки, і колір
    // «я» на одній та на другій зобов'язаний збігатись. Раніше кожен літерал
    // ARGB стояв у своєму файлі -- шість штук на дві карти.
    static const int MARK_SELF   = ARGB(255, 255, 122,  26);
    static const int MARK_BEACON = ARGB(255, 126, 200, 160);
    static const int MARK_ROUTE  = ARGB(255, 255, 170,  80);
    static const int MARK_PLAIN  = ARGB(255, 214, 214, 222);

    // Скільки тримається підказка від СЕРВЕРА, поки чергова перемальовка
    // не має права її затерти. Сторінки, що перепитують себе раз на
    // секунду, інакше губили відмови: між написанням і затиранням
    // проходила частка секунди.
    static const int HINT_HOLD_MS = 4000;
}

// Ідентифікатори слотів -- РОЗВ'ЯЗАНІ ОДИН РАЗ ЗА ЗАПУСК.
//
// InventorySlots.GetSlotIdFromString -- нативний пошук по таблиці всіх слотів
// гри за іменем, і мод кликав його НА КОЖНЕ звернення до вкладеного: обхід
// трьох відсіків у OZ_ModuleClass робить це тричі, а сам OZ_ModuleClass
// питають по п'ятнадцять-двадцять разів на один статус пристрою. Імена слотів
// -- константи, таблиця слотів у рантаймі не міняється, тож відповідь можна
// дати один раз і більше не питати.
//
// Ліниво, а не в OnMissionStart: клієнтові ці ж числа потрібні так само
// (худ шукає надітий прилад), а серверний старт до нього не доходить.
class OZ_PdaSlots
{
    private static bool s_Done = false;
    private static int  s_Battery = -1;
    private static int  s_Carrier = -1;
    private static int  s_Wear    = -1;
    private static ref array<int> s_Module;

    // НЕВДАЛИЙ ПОШУК НЕ КЕШУЄТЬСЯ, і це не педантизм.
    //
    // Кеш замикався ПЕРШИМ дотиком і запам'ятовував усе, що встиг знайти --
    // разом із -1. На клієнті першим питає HUD, двічі на секунду з першого ж
    // такту місії, тобто РАНІШЕ, ніж рушій зареєстрував таблицю слотів: у
    // s_Wear лягала -1 і лежала там до кінця сеансу. Наслідок був невидимий,
    // бо тодішній резолвер відкочувався на руки й малював худ із них
    // (ТЗ-5 R-B1.2 саме цей відкат і знімає) -- прибрали відкат, і худ
    // замовк назавжди. Зміряно на стенді 2026-09-07: із КПК у слоті носіння
    // сервер його бачив, клієнт -- ні.
    //
    // Тепер замикаємось лише тоді, коли знайшлось УСЕ; доти кожен виклик
    // перепитує рушій. Ціна -- кілька нативних викликів на перших секундах
    // місії, поки таблиця не з'явиться.
    private static void Ensure()
    {
        if (s_Done)
            return;

        if (s_Battery == -1)
            s_Battery = InventorySlots.GetSlotIdFromString(OZ_PdaConst.SLOT_BATTERY);
        if (s_Carrier == -1)
            s_Carrier = InventorySlots.GetSlotIdFromString(OZ_PdaConst.SLOT_CARRIER);
        if (s_Wear == -1)
            s_Wear = InventorySlots.GetSlotIdFromString(OZ_PdaConst.SLOT_WEAR);

        if (!s_Module)
        {
            s_Module = new array<int>();
            for (int f = 0; f < OZ_PdaConst.MODULE_SLOTS_MAX; f++)
                s_Module.Insert(-1);
        }

        bool all = true;
        if (s_Battery == -1)
            all = false;
        if (s_Carrier == -1)
            all = false;
        if (s_Wear == -1)
            all = false;

        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            if (s_Module[i] != -1)
                continue;

            s_Module.Set(i, InventorySlots.GetSlotIdFromString(OZ_PdaConst.ModuleSlot(i)));
            if (s_Module[i] == -1)
                all = false;
        }

        if (all)
            s_Done = true;
    }

    static int Battery() { Ensure(); return s_Battery; }
    static int Carrier() { Ensure(); return s_Carrier; }
    static int Wear()    { Ensure(); return s_Wear; }

    static int Module(int i)
    {
        Ensure();
        if (i < 0 || i >= s_Module.Count())
            return -1;
        return s_Module[i];
    }

    // За іменем -- для тих, хто вже має рядок. Ладер із п'яти порівнянь
    // замість нативного пошуку по таблиці гри; чуже ім'я йде до рушія, як і
    // раніше, і не кешується: воно тут не наше.
    static int Of(string name)
    {
        Ensure();

        if (name == OZ_PdaConst.SLOT_BATTERY)
            return s_Battery;
        if (name == OZ_PdaConst.SLOT_CARRIER)
            return s_Carrier;
        if (name == OZ_PdaConst.SLOT_WEAR)
            return s_Wear;

        for (int i = 0; i < s_Module.Count(); i++)
        {
            if (name == OZ_PdaConst.ModuleSlot(i))
                return s_Module[i];
        }

        return InventorySlots.GetSlotIdFromString(name);
    }
}
