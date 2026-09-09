// Форми відповідей сторінок КПК.
//
// Живуть у 3_Game, бо їх серіалізує сервер (4_World) і читає клієнт
// (5_Mission) -- спільним для обох є лише цей шар.
//
// ------------------------------------------------------------------------
// НАВІЩО ТУТ Copy() (зміряно 2026-09-06, persistence-networking.md; ідіома
// -- у шапці OZ_ConfigBase ядра).
//
// JsonFileLoader -- обгортка над рідним JsonSerializer.ReadFromString.
// Корінь він заповнює на місці, а от УСЕ ВКЛАДЕНЕ виділяє сам: жоден
// скриптовий конструктор і жоден ініціалізатор поля на тих об'єктах не
// виконується, і член, якому серіалізатор не знайшов значення, -- сира
// пам'ять. Одразу після розбору сторінка ще обнулена й читається правильно;
// після НАСТУПНОГО ВИДІЛЕННЯ на тому місці лежать чужі байти.
//
// Конверт, який споживається в тому ж виклику, копії не потребує. Copy() тут
// мають рівно ті типи, що переживають свій розбір: сторінки КПК тримають їх
// у m_-полях і перемальовують з них через кадри, а HUD -- через хвилини.
// ------------------------------------------------------------------------

// Один модульний відсік, як його бачить клієнт.
class OZ_BayInfo
{
    int    Index    = 0;
    bool   Visible  = false;   // профіль може ховати зайві відсіки
    string ClassName = "";     // порожньо -- відсік вільний
    string Display  = "";
    string Kind     = "";

    // РЕСУРС ПЛАТИ (ТЗ-5 R-B2.9): скільки хвилин роботи в ній лишилось.
    // -1 -- у плати ресурсу немає взагалі (звичайна антена, GPS), і рядок
    // про нього мовчить. Нуль разом із Burnt -- вигоріла.
    int    LeftMin  = -1;
    // Вигоріла плата НАЗИВАЄТЬСЯ, а не зникає: OZ_ModuleClass не віддає її
    // нікому (мертва електроніка), тож без цього прапорця гніздо з
    // почорнілою платою виглядало б порожнім -- саме те, що R-B2.9
    // забороняє.
    bool   Burnt    = false;

    OZ_BayInfo Copy()
    {
        OZ_BayInfo c = new OZ_BayInfo();
        c.Index     = Index;
        c.Visible   = Visible;
        c.ClassName = ClassName;
        c.Display   = Display;
        c.Kind      = Kind;
        c.LeftMin   = LeftMin;
        c.Burnt     = Burnt;
        return c;
    }
}

// Відповідь на device/status.
//
// Тут ЛИШЕ про той пристрій, що в руках. Клієнту не потрібна вся таблиця
// профілів, а надіслати її означало б роздати кожному опис усіх тирів,
// включно з тими, яких він ніколи не побачить.
class OZ_PdaDeviceStatus
{
    // --- пристрій ---
    string ClassName   = "";
    string ProfileId   = "";
    string DisplayName = "";
    ref array<string> Pages;          // з чого будувати стрічку вкладок
    int    ModuleSlots = 0;

    // Тут було поле Virtual -- прапорець «КПК без предмета» (D132). Рішення
    // власника 2026-09-08: стан завжди описує РІЧ, і другого роду відповіді
    // в клієнта немає.

    // Мережевий id САМЕ того КПК, про який відповів сервер, і чи він у руках.
    //
    // Клієнт не має права шукати пристрій самотужки: сервер бере руки, а
    // потім інвентар, і якщо клієнт подивиться лише в руки -- він покаже
    // прев'ю порожнечі там, де сервер говорить про цілком конкретний
    // пристрій у рюкзаку. Одна правда, і вона приїжджає звідси.
    bool InHands = false;
    int  NetLow  = 0;
    int  NetHigh = 0;

    // --- живлення ---
    //
    // HasBattery окремо від Charge01: порожнє гніздо й сіла батарея -- різні
    // біди, і кнопка живлення мусить писати різне.
    bool  Powered    = false;
    bool  HasBattery = false;
    float Charge01   = 0;

    // --- залізо ---
    ref array<ref OZ_BayInfo> Bays;
    string CarrierClass = "";
    bool   CarrierWritable = false;
    bool   CarrierWritten = false;
    // Капсула часу: вміст знімка і його дата -- лише коли пристрій офлайн.
    // Ім'я власника сесії -- завжди, коли сесія є (пристрій розімкнено).
    string Snapshot  = "";
    string OwnerName = "";
    // Чи Є в пристрою власник узагалі: без нього сторінка пропонує
    // ІНІЦІАЦІЮ, з чужим -- скидання.
    bool   Owned = false;
    // Секції нарізно: -1 -- секції немає. Стелі класу поруч, 0 -- безліміт.
    int    CarrierMarks = -1;
    int    CarrierNotes = -1;
    int    CarrierMaxRecords  = 0;
    // Скільки одиниць на чипі; -1 -- невідомо (чужий род або старий запис).

    // --- замок ---
    bool HasPin   = false;
    bool Unlocked = true;
    bool AutoLock = true;
    // Сервер може заборонити вимикати автоблокування. Клієнту це треба, щоб
    // не малювати кнопку, яка завжди відмовляє.
    bool ForceAutoLock = false;

    // --- запечатаний пристрій ---
    //
    // Sealed і LockedOut -- РІЗНІ речі, хоч обидва означають «код не
    // допоможе»: перший через те, що коду ніхто не знає, другий через те, що
    // спроби вичерпані. Гравцеві треба сказати, котре з двох.
    bool  Sealed       = false;
    bool  HasDecryptor = false;
    bool  Cracking     = false;
    int   CrackLeftSec = 0;
    bool LockedOut = false;           // спроби вичерпані
    int  LockWaitS = 0;               // секунд до кінця блокування; 0 -- без вікна
    float LockAfterMinutes = 0;
    // Скільки цифр у коді цієї моделі (ТЗ-5 R-B3.3). Пад малює рівно
    // стільки крапок і стільки ж цифр приймає. Нуль -- відповідь від
    // сервера, який про це поле ще не знає: клієнт бере умовчання.
    int  PinLength = 0;

    // --- сесія ---
    bool   Online     = false;        // епохи збігаються
    bool   SessionMine = false;       // сесія відкрита саме мною
    string SnapshotAt = "";           // коли знімок замерз; порожньо -- живий

    // --- прив'язка ---
    bool   DiscordLinked = false;

    // ЧОТИРЬОХ ПОЛІВ РАДІАЦІЇ ТУТ БІЛЬШЕ НЕМАЄ (рішення власника 2026-09-09):
    // радіометр і дозиметр прибрані з мода, а без них цю капсулу не заповнює
    // ніхто і не читає ніхто.

    void OZ_PdaDeviceStatus()
    {
        Pages = new array<string>();
        Bays  = new array<ref OZ_BayInfo>();
    }

    // Сторінка пристрою тримає цей об'єкт у m_Status і читає з нього на
    // кожному кліку кнопки живлення й автоблокування -- через кадри після
    // розбору. Меню КПК так само будує з Pages стрічку вкладок, створюючи
    // віджет на кожну.
    OZ_PdaDeviceStatus Copy()
    {
        OZ_PdaDeviceStatus c = new OZ_PdaDeviceStatus();
        c.ClassName   = ClassName;
        c.ProfileId   = ProfileId;
        c.DisplayName = DisplayName;
        c.ModuleSlots = ModuleSlots;
        c.InHands     = InHands;
        c.NetLow      = NetLow;
        c.NetHigh     = NetHigh;
        c.Powered     = Powered;
        c.HasBattery  = HasBattery;
        c.Charge01    = Charge01;
        c.CarrierClass    = CarrierClass;
        c.CarrierWritable = CarrierWritable;
        c.CarrierWritten  = CarrierWritten;
        c.Snapshot   = Snapshot;
        c.OwnerName  = OwnerName;
        c.Owned      = Owned;
        c.CarrierMarks = CarrierMarks;
        c.CarrierNotes = CarrierNotes;
        c.CarrierMaxRecords = CarrierMaxRecords;
        c.HasPin        = HasPin;
        c.Unlocked      = Unlocked;
        c.AutoLock      = AutoLock;
        c.ForceAutoLock = ForceAutoLock;
        c.Sealed       = Sealed;
        c.HasDecryptor = HasDecryptor;
        c.Cracking     = Cracking;
        c.CrackLeftSec = CrackLeftSec;
        c.LockedOut    = LockedOut;
        c.LockWaitS    = LockWaitS;
        c.LockAfterMinutes = LockAfterMinutes;
        c.PinLength    = PinLength;
        c.Online      = Online;
        c.SessionMine = SessionMine;
        c.SnapshotAt  = SnapshotAt;
        c.DiscordLinked = DiscordLinked;

        int i;
        if (Pages)
        {
            for (i = 0; i < Pages.Count(); i++)
                c.Pages.Insert(Pages[i]);
        }

        if (Bays)
        {
            for (i = 0; i < Bays.Count(); i++)
            {
                if (Bays[i])
                    c.Bays.Insert(Bays[i].Copy());
                else
                    c.Bays.Insert(new OZ_BayInfo());
            }
        }

        return c;
    }
}

// --- дрібні операції, які клієнт надсилає на сервер ---
//
// Кожна -- окремий тип, а не мапа рядків: поле з назвою Pin у сигнатурі
// видно на очі, а ["pin"] у мапі не видно нікому, поки не зламається.

class OZ_PdaPinAttempt
{
    string Pin = "";
}

class OZ_PdaPinChange
{
    string OldPin = "";
    string NewPin = "";
}

class OZ_PdaFlagOp
{
    bool Value = false;
}

// --- сторінка «Контакти» ---

class OZ_ContactEntry
{
    string Name = "";

    // ХТО ЦЕ -- окремо від того, ЯК ЙОГО ЗВУТЬ.
    //
    // Раніше людину в списку впізнавали по імені, і це ламалось двічі. Двоє
    // з однаковим ім'ям -- а ім'я в DayZ вибирає сам гравець -- і «викреслити
    // з друзів» викреслювало першого-ліпшого з двох. А контакт, чиє ім'я ще
    // не встигло кешуватись, малювався як «---» і не піддавався взагалі
    // нічому: ні написати, ні прибрати.
    //
    // Ключ -- хеш steam-id, не сам id: клієнту нема чого знати steam-id своїх
    // контактів, а щоб відрізнити двох у ВЛАСНОМУ списку друзів, хеша
    // вистачає з головою.
    string Key = "";
    bool   Me   = false;   // це ти

    // Коли його востаннє бачили в Зоні, UTC-рядком. Порожньо -- він тут
    // просто зараз, або жодного разу не заходив.
    //
    // Є в КОЖНОГО контакту, і саме тому по ньому нічого не видно: людина,
    // яка не з'являлась місяць, і людина, чий персонаж загинув, виглядають
    // однаково. КПК про смерть не повідомляє (рішення власника 2026-08-30)
    // -- він показує лише те, що знає: коли її бачили востаннє.
    string LastSeen = "";

    // Стан стосунків, рядком: "" -- ніхто, "friend" -- прийнятий друг,
    // "sent" -- я попросив і чекаю, "got" -- попросили мене,
    // "near" -- поруч, можна попросити.
    //
    // Рядком, а не набором булевих: станів п'ять і вони взаємовиключні, а
    // п'ять булевих дозволяють двадцять неможливих комбінацій.
    string Rel = "";

    // ДВІ ОСІ, вже людськими назвами: клієнту нема чого знати id, а сервер
    // уже має під рукою і те, і те (ТЗ-1 §5).
    //
    // Base є в кожного й каже, ХТО людина: «Сталкер». Org -- угруповання,
    // порожнє в одинака. Разом вони й дають рядок «Сталкер-легенда · Долг»
    // із приймання 12.6: базова половина йде в підпис, організаційна -- у
    // кольорову позначку.
    string Base = "";
    string Org  = "";

    // КОЛІР, готовим ARGB. Не слаг -- по слагу клієнт кольору не дістане:
    // OZ_Factions -- служба СЕРВЕРА, її таблиця на клієнті порожня, і
    // ColorARGB там повертає біле. Видно було одразу: смужка фракції Боргу
    // вийшла білою замість червоної.
    //
    // Колір УГРУПОВАННЯ: базова є в усіх, і фарбувати за нею означало б
    // пофарбувати весь список одним відтінком.
    int OrgColor = 0;

    // Чи він зараз у Зоні. Раніше цього не було, бо в списку були самі
    // присутні; тепер контакт лишається в списку й офлайн, і мовчазний
    // порожній рядок означав би «поруч».
    bool Online = false;

    // Чи в межах простягнутої руки просто зараз. Просити в друзі можна лише
    // зблизька, і кнопку слід малювати лише тоді.
    bool Near = false;

    // Звання, посади й мітки -- ВЖЕ людськими назвами, як і фракція, і з тієї
    // ж причини: слаги -- внутрішня справа, а перекладати їх на екрані мусив
    // би кожен, хто малює.
    //
    // Приїжджали від моста з першого дня й не малювались НІДЕ: лідер фракції
    // на екрані був не відрізнити від новачка, а мітка «Механік» не давала
    // нічого й нікому. Знайдено аудитом, а не в грі.
    string Rank = "";
    ref array<string> Traits;

    // Чи він у МОЄМУ УГРУПОВАННІ. Окремим полем, а не порівнянням назв:
    // назви задає адмін, і дві однакові -- його право, а не привід
    // зарахувати чужого в свої. Базова тут ні до чого: вона є в усіх, і
    // «свій» по ній означало б увесь сервер.
    bool Mine = false;

    // Поля «схований» тут НЕМАЄ навмисно. Для чужих воно завжди false (бо
    // схований у список не потрапляє зовсім), а для себе відповідь уже є --
    // MeHidden у самому списку. Друге поле про те саме означало б два місця,
    // де це може розійтись.

    // Конструктор ПІСЛЯ полів. Оголошений перед ними, він валить компіляцію
    // всього модуля World, і повідомлення про це не згадує ані цей клас, ані
    // конструктор: сипляться «Bad type 'JsonFileLoader'» по всіх файлах, які
    // цей тип серіалізують.
    void OZ_ContactEntry()
    {
        Traits = new array<string>();
    }

    OZ_ContactEntry Copy()
    {
        OZ_ContactEntry c = new OZ_ContactEntry();
        c.Name     = Name;
        c.Key      = Key;
        c.Me       = Me;
        c.LastSeen = LastSeen;
        c.Rel      = Rel;
        c.Base     = Base;
        c.Org      = Org;
        c.OrgColor = OrgColor;
        c.Online   = Online;
        c.Near     = Near;
        c.Rank     = Rank;
        c.Mine     = Mine;

        if (Traits)
        {
            for (int i = 0; i < Traits.Count(); i++)
                c.Traits.Insert(Traits[i]);
        }

        return c;
    }
}

// Лічильника окремим полем НЕМАЄ навмисно: він дорівнює довжині списку, а
// будь-яке інше число підказало б, що когось приховано.
class OZ_ContactList
{
    // Два вимикачі (ТЗ-4 R-A1.1): від Зони і від записників контактів.
    bool MeHiddenZone     = false;
    bool MeHiddenContacts = false;

    // Чи я лідер своєї фракції -- від цього залежить, чи малювати лідерські
    // кнопки взагалі. Рішення сервера: клієнт про свої права не здогадується.
    bool MeLeader = false;

    // Запрошення до фракції, яке чекає на МЕНЕ. Порожня назва -- немає.
    //
    // Живе в списку, а не в записі контакту: воно про мене, а не про когось
    // із рядка, і показувати його треба незалежно від того, кого я обрав.
    string InviteFaction = "";
    string InviteFrom    = "";

    // Чи проекція ролей протухла. Той, хто малює, мусить показати ОСТАННЄ
    // ВІДОМЕ приглушеним, а не порожнє: порожнє читається як «одинак», і
    // робити такий висновок гра права не має.
    //
    // Обчислювалось із першого дня й не питалось ніде.
    bool Stale = false;

    // Читальня капсули: імена з дайджеста замороженого пристрою. Людей
    // ПАМ'ЯТАЮТЬ, а не бачать: ні присутності, ні дій.
    bool Frozen = false;

    ref array<ref OZ_ContactEntry> Entries;

    void OZ_ContactList()
    {
        Entries = new array<ref OZ_ContactEntry>();
    }

    // Сторінка контактів тримає список у m_Data і читає з нього на кожному
    // кліку рядка й на кожному перемалюванні.
    OZ_ContactList Copy()
    {
        OZ_ContactList c = new OZ_ContactList();
        c.MeHiddenZone     = MeHiddenZone;
        c.MeHiddenContacts = MeHiddenContacts;
        c.MeLeader         = MeLeader;
        c.InviteFaction    = InviteFaction;
        c.InviteFrom       = InviteFrom;
        c.Stale            = Stale;
        c.Frozen           = Frozen;

        if (Entries)
        {
            for (int i = 0; i < Entries.Count(); i++)
            {
                if (Entries[i])
                    c.Entries.Insert(Entries[i].Copy());
                else
                    c.Entries.Insert(new OZ_ContactEntry());
            }
        }

        return c;
    }
}

// --- сторінка «Записки» ---
//
// Порожній Id означає «це нова записка». Так клієнту не треба знати, як
// сервер їх нумерує, і не треба другої операції «створити».

class OZ_Note
{
    string Id        = "";
    string Title     = "";
    string Body      = "";
    string CreatedAt = "";
    string EditedAt  = "";

    OZ_Note Copy()
    {
        OZ_Note c = new OZ_Note();
        c.Id        = Id;
        c.Title     = Title;
        c.Body      = Body;
        c.CreatedAt = CreatedAt;
        c.EditedAt  = EditedAt;
        return c;
    }
}

class OZ_NoteRef
{
    string Id = "";
}

// --- сторінка «Карта» ---
//
// Позиції рядком "x y z" -- у тому ж вигляді, у якому їх віддає vector, і в
// тому ж, у якому їх читає ToVector(). Окремих трьох полів тут не треба.

class OZ_MapBeacon
{
    string Name = "";
    string Pos  = "";

    // HUD тримає маячки в статику s_Beacons і малює міні-карту з них двічі
    // на секунду -- скільки завгодно довго після розбору посилки.
    OZ_MapBeacon Copy()
    {
        OZ_MapBeacon c = new OZ_MapBeacon();
        c.Name = Name;
        c.Pos  = Pos;
        return c;
    }
}

// СВОЄЇ ПОЗИЦІЇ ТУТ НЕМАЄ, і це не пропуск.
//
// Поле SelfPos сервер заповнював у кожній відповіді, а сторінка карти брала
// координати з ЛОКАЛЬНОЇ сутності гравця (LocalSelfPos) -- жива точка проти
// точки п'ятисекундної давнини. Читав його ніхто, а їхало воно завжди.
class OZ_MapState
{
    // Дальність транспондера в метрах -- умова і прийому, і передачі. Нуль
    // означає, що маячків немає взагалі, і це ОКРЕМИЙ стан, а не порожній
    // список.
    //
    // ОКРЕМОГО ПРАПОРЦЯ ТУТ БІЛЬШЕ НЕМАЄ. Було двоє: HasAntenna і
    // AntennaRangeM, -- і перший був рівно `другий > 0`. Відколи дальність
    // дає запис GPS (рішення власника 2026-09-09), «є чим вести» -- це
    // HasGps плюс це число, і зайвий біт лише давав би їм змогу розійтись.
    float TransponderRangeM = 0;

    // Куди дивиться мапа, коли прилад не знає, де він (ключ MapHome у
    // Tuning.json, «x z»). Числом сюди, а не в клієнтський конфіг: адмінське
    // значення живе на СЕРВЕРІ, а клієнтський OZ_PdaTuning знає лише
    // поставочні. Порожньо -- клієнт бере центр карти сам.
    string MapHome = "";

    // Кому цей прилад показує свою позицію -- НАБІР (ТЗ-4 R-A3.2):
    // ["public"] або будь-яка комбінація "faction" / "contacts".
    //
    // ПОРОЖНЬОГО НАБОРУ («вимкнено») БІЛЬШЕ НЕ БУВАЄ: рішення власника
    // 2026-09-09 прибрало вимикач -- транспондер веде завжди, коли прилад
    // увімкнений і має GPS, а гравець обирає лише коло глядачів. Збережене
    // «вимкнено» лікує ремонт при читанні файла (OZ_PdaAudience).
    ref array<string> TransponderSet;

    // Чи є на сервері мод фракцій: без нього перемикача "faction" на клієнті
    // не існує (R-A3.4). Рішення сервера.
    bool FactionsPresent = false;

    // Чи знає прилад, де він (модуль GPS, ТЗ-4 R-B2.2): без нього немає «ти
    // тут» і відстаней, а мітки й маршрут є. Приладу, що знав би це завжди
    // й без модуля -- віртуального термінала -- більше немає: рішення
    // власника 2026-09-08 прибрало його як єдиний виняток з-під воріт
    // (OZ_PdaMap.c, функціональний реєстр §2.6, D132). Це поле тепер про
    // КОЖЕН прилад означає рівно одне й те саме.
    bool HasGps = false;

    // Капсула (ТЗ-4 R-B1): живих маячків і живої позиції немає, є лише
    // записане на приладі.
    bool Frozen = false;

    ref array<ref OZ_MapBeacon> Beacons;

    // Мітки цього ПРИСТРОЮ і скільки їх іще влізе. Другий лічильник тут не
    // зайвий: «більше не влізе» гравець має дізнатись до того, як натисне.
    ref array<ref OZ_MapMarker> Markers;
    int MarkerLimit = 0;

    // Маршрут пристрою: впорядковані копії міток. Активація -- справа
    // клієнта; тут лише дані.
    ref array<ref OZ_MapMarker> Route;

    void OZ_MapState()
    {
        TransponderSet = new array<string>();
        Beacons = new array<ref OZ_MapBeacon>();
        Markers = new array<ref OZ_MapMarker>();
        Route   = new array<ref OZ_MapMarker>();
    }

    // Сторінка карти тримає стан у m_State і читає з нього все: мітки на
    // кожному перемалюванні, маршрут -- ще й після того, як його точки
    // поїхали в статик OZ_PdaRoute на HUD.
    OZ_MapState Copy()
    {
        OZ_MapState c = new OZ_MapState();
        c.TransponderRangeM = TransponderRangeM;
        c.MapHome         = MapHome;
        c.FactionsPresent = FactionsPresent;
        c.HasGps          = HasGps;
        c.Frozen          = Frozen;
        c.MarkerLimit     = MarkerLimit;

        int i;
        if (TransponderSet)
        {
            for (i = 0; i < TransponderSet.Count(); i++)
                c.TransponderSet.Insert(TransponderSet[i]);
        }

        if (Beacons)
        {
            for (i = 0; i < Beacons.Count(); i++)
            {
                if (Beacons[i])
                    c.Beacons.Insert(Beacons[i].Copy());
            }
        }

        if (Markers)
        {
            for (i = 0; i < Markers.Count(); i++)
            {
                if (Markers[i])
                    c.Markers.Insert(Markers[i].Copy());
            }
        }

        if (Route)
        {
            for (i = 0; i < Route.Count(); i++)
            {
                if (Route[i])
                    c.Route.Insert(Route[i].Copy());
            }
        }

        return c;
    }
}

class OZ_TransponderOp
{
    // Набір слагів, як у файлі гравця (ТЗ-4 R-A3.2). Порожній набір більше
    // не означає «вимкнути»: сервер підставляє замість нього поставочне
    // коло глядачів (OZ_PdaConst.TRANS_DEFAULT).
    ref array<string> Set;

    void OZ_TransponderOp()
    {
        Set = new array<string>();
    }
}

// Посилання на людину ІМЕНЕМ, а не Steam64. Клієнт чужого id не бачить і не
// має бачити; сервер сам вирішує, кому це ім'я належить -- і серед кого саме
// шукати (поруч, у друзях, серед запитів).
// Посилання на людину або на рядок тексту.
//
// Key -- коли йдеться про КОНТАКТА (див. OZ_ContactEntry.Key). Name лишається
// для того, що ім'ям і є насправді: назви нової групи, наприклад.
class OZ_NameRef
{
    string Name = "";
    string Key  = "";
}

// --- мітки ---
//
// Мітки живуть НА ПРИСТРОЇ, а не в акаунті -- на відміну від записок. Через
// це межа Limits.Memory у профілі щось означає (кращий КПК тримає більше),
// через це має сенс носій даних, і через це вкрадений КПК віддає чужі
// схованки. Усе три -- навмисно.

class OZ_MapMarker
{
    string Id   = "";
    string Name = "";
    string Pos  = "";

    // Опис -- довше за назву й не для карти: на карті він був би кашею.
    // Живе в списку міток і редагується там само. Старі записи без поля
    // читаються як порожній опис -- JsonFileLoader незнайоме поле не чіпає,
    // а відсутнє лишає замовчуванням.
    string Desc = "";

    OZ_MapMarker Copy()
    {
        OZ_MapMarker c = new OZ_MapMarker();
        c.Id   = Id;
        c.Name = Name;
        c.Pos  = Pos;
        c.Desc = Desc;
        return c;
    }
}

class OZ_MarkerList
{
    ref array<ref OZ_MapMarker> Items;

    void OZ_MarkerList()
    {
        Items = new array<ref OZ_MapMarker>();
    }

    // Список міток виходить із розбору живим: LoadMarkers віддає його
    // викликачеві, той дописує, ріже й серіалізує назад на пристрій -- усе
    // це після нових виділень.
    OZ_MarkerList Copy()
    {
        OZ_MarkerList c = new OZ_MarkerList();
        if (Items)
        {
            for (int i = 0; i < Items.Count(); i++)
            {
                if (Items[i])
                    c.Items.Insert(Items[i].Copy());
            }
        }
        return c;
    }
}

class OZ_MarkerRef
{
    string Id = "";
}

// --- носій даних ---
//
// Що писати на чип. Kind -- "markers" або "notes"; порожні списки
// означають «усе, що є».
// Відповідь імпорту: скільки взяли проти скільки лежало. Різниця між ними
// -- те, що НЕ влізло, і гравець мусить це побачити, а не почути "Done.".
class OZ_CarrierTaken
{
    int Taken = 0;
    int Total = 0;
}

class OZ_CarrierWriteOp
{
    string Kind = "";
}

// --- сторінка «Зв'язок» ---
//
// По проводу їдуть ІМЕНА, а не Steam64: клієнт чужих id не бачить ніде, і
// чат тут не виняток.

// Рядок переліку розмов. Лічильника повідомлень тут немає навмисно: у
// Discord їх стільки, скільки їх там є, а міст тримає лише хвіст -- назвати
// довжину хвоста «кількістю повідомлень» означало б збрехати.
class OZ_ChatHead
{
    string Id       = "";
    string Kind     = "direct";
    string Title    = "";
    // Desc, LastAt і LastText МІСТ шле, а ця сторінка не малює. Поля
    // лишаються описом його конверта, а не нашим навантаженням: тіло
    // v1/chat/list іде на клієнт як є, і зняти їх звідси означало б лише
    // перестати їх РОЗБИРАТИ, не заощадивши жодного байта.
    string Desc     = "";
    string LastAt   = "";
    string LastText = "";

    OZ_ChatHead Copy()
    {
        OZ_ChatHead c = new OZ_ChatHead();
        c.Id       = Id;
        c.Kind     = Kind;
        c.Title    = Title;
        c.Desc     = Desc;
        c.LastAt   = LastAt;
        c.LastText = LastText;
        return c;
    }
}

// Запрошення до групи, яке чекає на мене. Прийняти чи відхилити --
// МІЙ клік, а не чужий: у групу ніхто не потрапляє мовчки.
class OZ_ChatInvite
{
    string Id    = "";
    string Title = "";
    string From  = "";

    OZ_ChatInvite Copy()
    {
        OZ_ChatInvite c = new OZ_ChatInvite();
        c.Id    = Id;
        c.Title = Title;
        c.From  = From;
        return c;
    }
}

class OZ_ChatList
{
    // Читальня капсули: список приїхав зрізом, нового не завести.
    bool Frozen = false;
    ref array<ref OZ_ChatHead> Items;
    ref array<ref OZ_ChatInvite> Invites;

    void OZ_ChatList()
    {
        Items   = new array<ref OZ_ChatHead>();
        Invites = new array<ref OZ_ChatInvite>();
    }

    // Сторінка чату тримає перелік у m_Heads і питає його на кожному пуші
    // (HeadKnown) та на кожному перемалюванні.
    OZ_ChatList Copy()
    {
        OZ_ChatList c = new OZ_ChatList();
        c.Frozen = Frozen;

        int i;
        if (Items)
        {
            for (i = 0; i < Items.Count(); i++)
            {
                if (Items[i])
                    c.Items.Insert(Items[i].Copy());
            }
        }

        if (Invites)
        {
            for (i = 0; i < Invites.Count(); i++)
            {
                if (Invites[i])
                    c.Invites.Insert(Invites[i].Copy());
            }
        }

        return c;
    }
}

class OZ_ChatLine
{
    string At   = "";
    // Слід автора для СЕРВЕРА: міст кладе сюди Steam64, сервер міняє
    // його на колір фракції і СТИРАЄ -- клієнтові чужі id не дістаються.
    string AUid = "";
    // ARGB фракції автора; 0 -- без фарби.
    int WhoColor = 0;
    string Who  = "";
    string Text = "";
    bool   Mine = false;

    OZ_ChatLine Copy()
    {
        OZ_ChatLine c = new OZ_ChatLine();
        c.At       = At;
        c.AUid     = AUid;
        c.WhoColor = WhoColor;
        c.Who      = Who;
        c.Text     = Text;
        c.Mine     = Mine;
        return c;
    }
}

class OZ_ChatView
{
    string Id    = "";
    string Kind  = "direct";
    string Title = "";
    string Desc  = "";
    // Чи є що вантажити ГЛИБШЕ, і якір найстарішого показаного рядка.
    // Якір непрозорий: клієнт лише повертає його в "older" як є.
    bool   More = false;
    string Before = "";
    // Читальня капсули: рядки зрізані по заморозці, поле вводу мертве.
    bool   Frozen = false;
    // Чи Я створив цю групу: ключ групи назавжди носить ім'я засновника,
    // і лише він її видаляє; решта -- виходять.
    bool   Owner = false;
    ref array<ref OZ_ChatLine> Lines;
    ref array<string> Members;

    void OZ_ChatView()
    {
        Lines   = new array<ref OZ_ChatLine>();
        Members = new array<string>();
    }

    // Найдовгоживучіший конверт КПК: розмова лежить у m_View весь час, поки
    // вікно відкрите, а «дай старіше» ще й вставляє нові рядки в її початок.
    OZ_ChatView Copy()
    {
        OZ_ChatView c = new OZ_ChatView();
        c.Id     = Id;
        c.Kind   = Kind;
        c.Title  = Title;
        c.Desc   = Desc;
        c.More   = More;
        c.Before = Before;
        c.Frozen = Frozen;
        c.Owner  = Owner;

        int i;
        if (Lines)
        {
            for (i = 0; i < Lines.Count(); i++)
            {
                if (Lines[i])
                    c.Lines.Insert(Lines[i].Copy());
            }
        }

        if (Members)
        {
            for (i = 0; i < Members.Count(); i++)
                c.Members.Insert(Members[i]);
        }

        return c;
    }
}

class OZ_ChatRef
{
    string Id = "";
}

class OZ_ChatSend
{
    string Id   = "";
    string Text = "";

    // Тільки для «Зони»: сказати в ефір без імені. Міст сам відмовить
    // будь-якій іншій розмові -- там співрозмовник обирав, З КИМ говорить.
    bool Anon = false;
}

class OZ_ChatAdd
{
    string Id   = "";
    string Name = "";
    string Key  = "";
}

// Один запис на носії, на який вказує гравець: секція і місце в ній.
class OZ_CarrierItemRef
{
    string Kind = "";   // "mark" | "note"
    int    Index = -1;
}

// Кого можна покликати в групу: імена контактів гравця. Відповідь сторінки
// чату, бо саме їй потрібна -- меню маршрутизує відповіді за сторінкою.
class OZ_ChatInvitees
{
    ref array<string> Names;

    void OZ_ChatInvitees()
    {
        Names = new array<string>();
    }
}

// Назва й опис групи одним листом: Id порожній -- створити, заданий --
// правити існуючу.
class OZ_ChatGroupSpec
{
    string Id   = "";
    string Name = "";
    string Desc = "";
}

// «Дай старіше»: якір -- Before із попередньої відповіді.
class OZ_ChatOlderReq
{
    string Id     = "";
    string Before = "";
}

// Знімок для капсули часу: що замерзне на пристрої, коли власник заведе
// новий. Пише сервер із живої сесії, читає сторінка пристрою офлайн-капсули.
class OZ_PdaSnapshot
{
    string Owner   = "";
    // ОБИДВІ ОСІ, і саме тут це важить найбільше: капсула -- це те, ким
    // людина БУЛА. Записати одне поле означало б, що з двох правд про
    // померлого лишиться навмання одна.
    string Base    = "";
    string Org     = "";
    ref array<string> Contacts;

    void OZ_PdaSnapshot()
    {
        Contacts = new array<string>();
    }

    OZ_PdaSnapshot Copy()
    {
        OZ_PdaSnapshot c = new OZ_PdaSnapshot();
        c.Owner = Owner;
        c.Base  = Base;
        c.Org   = Org;

        if (Contacts)
        {
            for (int i = 0; i < Contacts.Count(); i++)
                c.Contacts.Insert(Contacts[i]);
        }

        return c;
    }
}
