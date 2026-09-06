// Tuning.json -- ігрові ходові ручки КПК одним файлом.
//
// Все, що адмін хоче підкрутити МІЖ рестартами, живе тут, а не в
// константах: межі текстів, лічильники спроб, радіуси, секунди. Поставочні
// числа -- ЦЕ САМІ ПОЛЯ цього класу й більш ніщо: другий комплект у
// OZ_PdaConst знято 2026-09-06, бо ніщо не звіряло його з першим.
//
// Частина значень потрібна КЛІЄНТОВІ (тривалість тоста, радіус автопроходу
// маршруту) -- їх він отримує не звідси, а в конверті beacon-пуша:
// сервер раз на тік докладає два числа, і окремий канал не потрібен.

class OZ_PdaTuning : OZ_ConfigBase
{
    // --- PIN ---
    // Скільки невдалих спроб підряд замикає пристрій для цієї особи.
    int PinMaxFails = 5;
    // На скільки секунд. 0 -- до рестарту сервера (стара поведінка).
    int PinLockoutSeconds = 300;

    // --- chat ---
    int ChatMsgMaxBytes   = 1000;
    int ChatTitleMaxBytes = 32;
    int ChatDescMaxBytes  = 96;
    // Скільки останніх рядків віддає міст при відкритті розмови...
    int ChatHistoryOpen = 20;
    // ...і скільки довантажує LOAD OLDER за один крок.
    int ChatHistoryPage = 20;
    // Скільки живе запрошення до групи (ТЗ-4 R-D3.3): своє поле, своє
    // число -- фракційне (Faction.InviteTtlSeconds, 120 с) для групи
    // замале: пропозиція має пережити ніч. Міст прибирає протухлі сам.
    int GroupInviteTtlSeconds = 86400;
    // Стеля складу групи. 0 -- без межі. Перевіряє МІСТ при запрошенні.
    int ChatGroupMax = 16;

    // --- нотатки ---
    // Запасна стеля кількості, коли профіль пристрою не каже своєї.
    int NoteTitleMaxBytes = 64;
    // Не більше 1000: JsonFileLoader ріже рядки на 1023 байтах, і міст
    // кліпає на тій самій межі. Validate не дасть поставити більше.
    int NoteBodyMaxBytes = 1000;

    // --- мітки карти ---
    int MarkerNameMaxBytes = 32;
    int MarkerDescMaxBytes = 160;

    // --- контакти ---
    // З якої відстані інший гравець вважається «поруч» для обміну.
    int FriendReachMeters = 12;
    // Скільки живе пропозиція обміну контактами.
    int SwapOfferTtlSeconds = 60;

    // --- HUD (їде клієнтові в beacon-пуші) ---
    int ToastSeconds = 8;
    int RouteAdvanceMeters = 30;
    // Як часто сервер розсилає маячки власникам працюючих антен.
    int BeaconPushSeconds = 5;

    private static ref OZ_PdaTuning s_Inst;

    // НІКОЛИ НЕ null, і саме тому кожен читач нижче -- один рядок.
    //
    // Було: Get() віддавав null до ServerLoad, і сімнадцять читачів OZ_PdaTune
    // мали по три рядки на цей випадок, а поставочні числа лежали ДРУГИМ
    // комплектом у OZ_PdaConst -- два джерела однієї правди, які ніщо не
    // звіряло. Тепер джерело одне: поля цього класу. Клієнт, який ServerLoad
    // не кличе ніколи, дістає рівно ті самі поставочні значення.
    static OZ_PdaTuning Get()
    {
        if (!s_Inst)
        {
            s_Inst = new OZ_PdaTuning();
            s_Inst.LoadDefaults();
        }
        return s_Inst;
    }

    override int LatestVersion()
    {
        return 2;
    }

    override void LoadDefaults()
    {
        Version = LatestVersion();

        PinMaxFails       = 5;
        PinLockoutSeconds = 300;

        ChatMsgMaxBytes   = 1000;
        ChatTitleMaxBytes = 32;
        ChatDescMaxBytes  = 96;
        ChatHistoryOpen   = 20;
        ChatHistoryPage   = 20;
        ChatGroupMax      = 16;
        GroupInviteTtlSeconds = 86400;

        NoteTitleMaxBytes = 64;
        NoteBodyMaxBytes  = 1000;

        MarkerNameMaxBytes = 32;
        MarkerDescMaxBytes = 160;

        FriendReachMeters   = 12;
        SwapOfferTtlSeconds = 60;

        ToastSeconds       = 8;
        RouteAdvanceMeters = 30;
        BeaconPushSeconds  = 5;
    }

    override bool Migrate(int from)
    {
        // v2: блокування піна стало в СЕКУНДАХ (PinLockoutSeconds замість
        // хвилин). Старе поле не переноситься: схема прожила лічені години,
        // і значення за замовчуванням ті самі 5 хвилин.
        Version = LatestVersion();
        return true;
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        warnings += ClampMin("PinMaxFails", PinMaxFails, 1);
        warnings += ClampMin("PinLockoutSeconds", PinLockoutSeconds, 0);

        // Вище 1000 байтів не можна НІДЕ, де текст їздить через
        // JsonFileLoader чи міст: різ на 1023 псує UTF-8 посеред знака.
        warnings += ClampMax("NoteBodyMaxBytes", NoteBodyMaxBytes, 1000);
        warnings += ClampMax("ChatMsgMaxBytes", ChatMsgMaxBytes, 1000);

        // СТЕЛІ ТЕКСТУ МАЮТЬ І ПІДЛОГУ, а не саму лише стелю.
        //
        // Нуль чи від'ємне в будь-якій із п'яти означало Substring(0, <=0) на
        // кожному імені й кожному тілі: назви міток порожніли мовчки, а
        // від'ємна довжина -- це вже питання до рушія. Чотири байти -- це
        // одна кирилична пара; менше не є назвою ні для чого.
        warnings += ClampMin("ChatMsgMaxBytes", ChatMsgMaxBytes, 16);
        warnings += ClampMin("ChatTitleMaxBytes", ChatTitleMaxBytes, 4);
        warnings += ClampMin("ChatDescMaxBytes", ChatDescMaxBytes, 4);
        warnings += ClampMin("NoteTitleMaxBytes", NoteTitleMaxBytes, 4);
        warnings += ClampMin("NoteBodyMaxBytes", NoteBodyMaxBytes, 8);
        warnings += ClampMin("MarkerNameMaxBytes", MarkerNameMaxBytes, 4);
        warnings += ClampMin("MarkerDescMaxBytes", MarkerDescMaxBytes, 4);

        warnings += ClampMin("ChatHistoryOpen", ChatHistoryOpen, 1);
        warnings += ClampMin("ChatHistoryPage", ChatHistoryPage, 1);
        warnings += ClampMin("ChatGroupMax", ChatGroupMax, 0);
        warnings += ClampMin("GroupInviteTtlSeconds", GroupInviteTtlSeconds, 60);

        warnings += ClampMin("FriendReachMeters", FriendReachMeters, 1);
        warnings += ClampMin("SwapOfferTtlSeconds", SwapOfferTtlSeconds, 5);

        warnings += ClampMin("ToastSeconds", ToastSeconds, 2);
        warnings += ClampMin("RouteAdvanceMeters", RouteAdvanceMeters, 5);
        warnings += ClampMin("BeaconPushSeconds", BeaconPushSeconds, 2);
    }

    // КЛАМП ЖИВЕ ТУТ, а не в викликача. Enforce ПЕРЕДАЄ int за посиланням --
    // ключове слово inout, -- тож коментар «сам кламп робить викликач» був
    // неправдою, яка коштувала чотирнадцяти зайвих рядків Math.Max поруч, і
    // кожен із них міг розійтися зі своїм попередженням непомітно.
    private int ClampMin(string name, inout int val, int floor)
    {
        if (val >= floor)
            return 0;
        OZ_Log.Warn("Tuning: " + name + " under " + floor.ToString() + ", clamped");
        val = floor;
        return 1;
    }

    private int ClampMax(string name, inout int val, int ceiling)
    {
        if (val <= ceiling)
            return 0;
        OZ_Log.Warn("Tuning: " + name + " over " + ceiling.ToString() + " breaks storage strings and the bridge clip, clamped");
        val = ceiling;
        return 1;
    }

    static void ServerLoad()
    {
        s_Inst = new OZ_PdaTuning();
        OZ_ConfigLoader<OZ_PdaTuning>.Load(OZ_Const.PROFILE_DIR + "\\OZ_PDA_Tuning.json", "Tuning", s_Inst);
    }
}

// Читачі одним іменем. Запасних значень тут БІЛЬШЕ НЕМАЄ: Get() ніколи не
// віддає null, а поставочні числа живуть у полях самого OZ_PdaTuning --
// один комплект замість двох, що розходились би мовчки.
class OZ_PdaTune
{
    static int PinMaxFails()      { return OZ_PdaTuning.Get().PinMaxFails; }
    static int PinLockoutMs()     { return OZ_PdaTuning.Get().PinLockoutSeconds * 1000; }

    static int ChatMsgMax()       { return OZ_PdaTuning.Get().ChatMsgMaxBytes; }
    static int ChatTitleMax()     { return OZ_PdaTuning.Get().ChatTitleMaxBytes; }
    static int ChatDescMax()      { return OZ_PdaTuning.Get().ChatDescMaxBytes; }
    static int ChatHistoryOpen()  { return OZ_PdaTuning.Get().ChatHistoryOpen; }
    static int ChatHistoryPage()  { return OZ_PdaTuning.Get().ChatHistoryPage; }
    static int GroupInviteTtlS()  { return OZ_PdaTuning.Get().GroupInviteTtlSeconds; }
    static int ChatGroupMax()     { return OZ_PdaTuning.Get().ChatGroupMax; }

    static int NoteTitleMax()     { return OZ_PdaTuning.Get().NoteTitleMaxBytes; }
    static int NoteBodyMax()      { return OZ_PdaTuning.Get().NoteBodyMaxBytes; }

    static int MarkerNameMax()    { return OZ_PdaTuning.Get().MarkerNameMaxBytes; }
    static int MarkerDescMax()    { return OZ_PdaTuning.Get().MarkerDescMaxBytes; }

    static int FriendReachM()     { return OZ_PdaTuning.Get().FriendReachMeters; }
    static int SwapOfferTtlMs()   { return OZ_PdaTuning.Get().SwapOfferTtlSeconds * 1000; }

    static int ToastSeconds()     { return OZ_PdaTuning.Get().ToastSeconds; }
    static int RouteAdvanceM()    { return OZ_PdaTuning.Get().RouteAdvanceMeters; }
    static float BeaconPushSeconds() { return OZ_PdaTuning.Get().BeaconPushSeconds; }
}
