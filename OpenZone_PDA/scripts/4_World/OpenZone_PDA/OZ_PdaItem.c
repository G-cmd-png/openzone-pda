// Предмет-КПК.
//
// Живлення -- рушійне: батарея це просто attachments[] у конфізі, а plugType=1
// плюс attachmentAction=1 змушують рушій самому втикати пристрій у вкладену
// батарею й висмикувати при вилученні. Скриптові лишається возити стан.
//
// ЗАМОК І СЕСІЯ -- РІЗНІ РЕЧІ, і в цьому вся механіка.
//
//   ЗАМОК належить ПРИСТРОЮ. Розблокований КПК працює в будь-чиїх руках --
//   передав сталкер свій відімкнений ПДА, і ним можна користуватись. Сам
//   запирається через N хвилин після того, як його прибрали з рук.
//
//   Щоб ЗМІНИТИ пін, його треба знати. Пристрій не питає, хто ти, він питає
//   старий код -- і автоблокування вимикає будь-хто, у кого КПК у руках.
//
//   СЕСІЯ належить ГРАВЦЕВІ й вирішує ЧИЇ дані видно, а не що можна натиснути.
//   Живе, поки епоха пристрою збігається з епохою гравця в його файлі.
//   «Скинути інші сесії» -- це епоха += 1, після чого решта пристроїв
//   переходять в ОФЛАЙН: не ламаються, а перестають оновлюватись, і їхній
//   знімок замерзає на цій миті.
//
// ПІН перевіряється ЗАВЖДИ на сервері й клієнтові не їде ніколи: інакше
// замок знімався б правкою пам'яті клієнта, тобто його не було б узагалі.

class OZ_PDA_Base : ItemBase
{
    private bool  m_IsOn     = false;
    private float m_Charge01 = 0;

    // Дзеркало «в приладі стоїть робочий GPS» для КЛІЄНТА, як m_HasPinS
    // нижче. Таблиця заліза (Hardware.json і `Kind` кожного класу) до клієнта
    // не доїжджає ВЗАГАЛІ -- вона читається в OnMissionStart за
    // `if (!GetGame().IsServer()) return` (функціональний реєстр §10 D82), тож
    // OZ_HasModuleKind на клієнті завжди відповідає «немає». Через це худ і
    // не міг спитати про GPS сам: мінікарта малювалась на приладі без
    // приймача (звіт власника 2026-09-09, дефект 2). Один біт замість
    // конвеєра таблиці: питання рівно одне -- знає прилад, де він, чи ні.
    private bool  m_HasGpsS  = false;

    // --- замок ---
    private string m_Pin        = "";     // порожній рядок = коду немає
    private bool   m_Unlocked   = false;  // стан ПРИСТРОЮ, не гравця
    // Дзеркало «пін задано» для КЛІЄНТА: сам пін секретний і на клієнт не
    // їде ніколи, а от факт його існування мусить бути видно без RPC --
    // від нього гаснуть худ і дія обміну на замкненому пристрої.
    private bool   m_HasPinS = false;
    // Зламаний квестовий прилад лишається зламаним назавжди (ТЗ-4 R-B3.2):
    // повторної печатки не буває, хоч би який код на нього потім ставили.
    // Зберігається (v7), інакше рестарт «запечатував» би його знову.
    private bool   m_Cracked = false;
    private bool   m_AutoLock   = true;   // можна вимкнути власником сесії
    private int    m_LeftHandsAt = 0;     // GetGame().GetTime() у мс, 0 = в руках

    // --- сесія ---
    private string m_SessionUid   = "";
    private int    m_SessionEpoch = 0;

    // --- знімок даних ---
    // Поки сесія жива, сервер оновлює цей знімок. Коли гравець скинув сесії
    // з іншого пристрою, епохи розходяться, оновлення припиняються -- і
    // знімок ЗАМЕРЗАЄ. Пристрій не втрачає даних і не стає цеглиною: він
    // стає офлайном. Зламати його можна, але побачити там світ таким, яким
    // він був у мить розлогіну, і ні секундою пізніше.
    //
    // Украдений КПК через це -- капсула часу, а не жива прослушка. Це і є
    // головна причина такої моделі.
    private string m_Snapshot   = "";
    private string m_SnapshotAt = "";

    // --- мітки ---
    //
    // Зберігаються РЯДКОМ JSON, а не масивом: запис CF позиційний, і масив
    // змінної довжини в ньому довелось би вести самому -- лічильник, потім
    // елементи, і будь-яка неузгодженість з'їдає потік усіх, хто пише після
    // нас. Один рядок такої проблеми не має.
    private string m_MarkersJson = "";
    // Записки -- теж пам'ять ПРИСТРОЮ (рішення власника 2026-08-28):
    // трофей при захопленні, природна капсула, жодного моста.
    private string m_NotesJson = "";
    // Маршрут: ВПОРЯДКОВАНИЙ список копій міток (той самий OZ_MarkerList).
    // Один на пристрій: вести дві нитки одночасно однаково нема кому.
    private string m_RouteJson = "";

    // Розділи ЧУЖИХ МОДУЛІВ -- у ТОМУ Ж сховищі секцій, що й у носія
    // (рішення власника 2026-09-08). Формат запису -- там же, в OZ_SectionStore,
    // і саме тому він не може розійтися між приладом і носієм.
    //
    // Чому розділи взагалі МАСИВОМ, а не одним JSON-документом: документом
    // воно й було, і саме це валило сервер. Корисне навантаження розділу --
    // САМЕ ПО СОБІ JSON; загорнувши його в поле іншого JSON, ми отримали JSON
    // усередині JSON, тобто екранування при записі й розекранування при
    // читанні. Розібрати те, що звідти поверталось, JsonFileLoader не міг:
    // 2026-08-31 сервер помирав нативно ПРЯМО В LoadData -- лог обривався між
    // «payload NNN bytes» і наступним рядком.
    //
    // Носій цієї помилки не мав ніколи: він пише Kind, Records і Payload
    // ОКРЕМИМИ записами сховища й нічого ні в що не загортає. Тут так само --
    // тепер уже тим самим кодом.
    private ref OZ_SectionStore m_Store;

    // СКІЛЬКИ ЯЧЕЙОК ЗАЙНЯТО ВЛАСНИМИ РОДАМИ -- порахований раз і збережений.
    //
    // Число рахується розбором ОБОХ документів -- міток і записок, -- а
    // питають його на кожну операцію будь-якої сторінки: стеля міток, стеля
    // записок, вільне місце на екрані пристрою, кожен імпорт із чипа. Тобто
    // два повні розбори JSON по сорок записів на КОЖЕН клік, і ще на кожен
    // тік сторінки, і для кожного КПК на сервері.
    //
    // -1 означає «не рахували». Скидають його рівно ті три сеттери, що міняють
    // документи, плюс робота з розділами чужих модулів і заводський скид.
    private int m_OwnCells = -1;

    // --- запечатаний пристрій ---
    //
    // m_Seeded відповідає на одне питання: чи вже писали на цей пристрій те,
    // що профіль велів написати. Один раз -- і назавжди; інакше кожен рестарт
    // відновлював би мітки, які гравець стер, і квестова схованка ставала б
    // невичерпною.
    //
    // m_CrackUntil -- час у мілісекундах GetGame().GetTime(), після якого
    // код спаде. Нуль означає «ніхто не ламає». Час РУШІЙНИЙ, а не
    // календарний: рестарт сервера злічильник обнуляє, і це правильно --
    // дешифратор рахував у пам'яті, а не в сейфі.
    private bool m_Seeded     = false;
    private int  m_CrackUntil = 0;

    // --- тік модулів ---
    //
    // Один таймер на пристрій, і він біжить із НАЙМЕНШИМ періодом серед
    // поведінок прикріплених модулів -- а не з фіксованою чвертю секунди.
    //
    // Чверть секунди означала чотири спрацювання на секунду на КОЖНОМУ
    // ввімкненому КПК сервера, і кожне з них робило повний обхід трьох
    // відсіків: GetSlotIdFromString, FindAttachment, пошук профілю, пошук
    // специфікації заліза, пошук поведінки -- щоб у 19 випадках із 20
    // дійти до `m_ModuleAcc[i] < period` і не зробити нічого. Єдина
    // поведінка, яка сьогодні існує, просить п'ять секунд.
    //
    // Нуль означає «тікати нема кому»: жоден прикріплений модуль не має
    // поведінки з періодом. Тоді таймер СТОЇТЬ.
    //
    // НА КЛІЄНТІ це природно дає нуль, і в цьому друга половина правки.
    // ForClass іде через OZ_PdaHardware.ModuleFor, а той відповідає лише
    // коли заповнено s_Cfg -- а це робить ЛИШЕ ServerLoad(), викликаний
    // ЛИШЕ на сервері. Отже на клієнті ForClass не відповідає нічого
    // жодному класнейму, хоч би який мод зареєстрував свою поведінку --
    // клієнтський тік крутився вхолосту від першої версії, і сама
    // реєстрація поведінки клієнтську роботу не вмикає: без таблиці заліза
    // на клієнті нема кого шукати. Мод, який захоче клієнтської роботи
    // (звук детектора), мусить довезти на клієнт ще й Hardware.json (чи
    // принаймні Kind потрібного класу) -- цього конвеєра сьогодні немає.
    private ref Timer m_ModuleTimer;
    private ref array<float> m_ModuleAcc;
    // Період, з яким таймер біжить ЗАРАЗ. Нуль -- таймер не заведений.
    private float m_TickPeriod = 0;
    // Мить попереднього спрацювання, GetGame().GetTime() у мс.
    private int m_LastTickMs = 0;

    // --- лічильник невдалих спроб ---
    private ref array<string> m_FailUid;
    private ref array<int>    m_FailCount;
    // Мить ОСТАННЬОЇ невдачі: від неї рахується вікно блокування.
    private ref array<int>    m_FailAt;

    // Межі -- з Tuning.json (OZ_PdaTune.PinMaxFails / PinLockoutMs).

    void OZ_PDA_Base()
    {
        RegisterNetSyncVariableBool("m_IsOn");
        // (ім'я, мін, макс, точність у знаках після коми). Для 0..1 два знаки
        // -- це один відсоток, дрібніше ніхто не побачить.
        RegisterNetSyncVariableFloat("m_Charge01", 0, 1, 2);
        RegisterNetSyncVariableBool("m_Unlocked");
        RegisterNetSyncVariableBool("m_HasPinS");
        RegisterNetSyncVariableBool("m_HasGpsS");

        m_Store     = new OZ_SectionStore();

        m_FailUid   = new array<string>();
        m_FailCount = new array<int>();
        m_FailAt    = new array<int>();

        m_ModuleAcc = new array<float>();
        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
            m_ModuleAcc.Insert(0);
    }

    // --------------------------------------------------------------- модулі

    // Предмет щойно з'явився у світі. Те, що профіль велів на нього покласти,
    // пишеться НЕ ТУТ, а НАСТУПНИМ КАДРОМ -- і лише якщо його ще не писали.
    //
    // Причина в порядку подій. EEInit кличеться і при створенні, і при
    // завантаженні зі збереження, але СХОВИЩЕ ЧИТАЄТЬСЯ ПІЗНІШЕ: на цю мить
    // m_Seeded ще має значення за замовчуванням, тобто false, хоч на диску
    // лежить true. Тобто захисна защіпка «рівно один раз за життя предмета»
    // тут не працює взагалі й не може: вона питає поле, якого ще не читали.
    //
    // Досі це не було видно, бо КОЖНЕ поле, яке пише посів, лежить і в
    // сховищі, тож наступний OnStoreLoad затирав зайву роботу правильними
    // значеннями. Це збіг, а не захист: перше ж посіяне поле, яке не
    // зберігається, почало б мовчки скидатись при кожному рестарті сервера.
    //
    // Відкладений виклик знімає збіг: наступного кадру сховище вже прочитане,
    // і защіпка відповідає те, що написано на диску.
    // Відкладений посів. ВЛАСНИЙ таймер, а не спільна черга виклику.
    //
    // Через чергу це вже спробували: `CallLater(this.OZ_SeedFromProfile, ...)`
    // і зняття в деструкторі. Живий сервер відповів «Virtual Machine
    // Exception / NULL pointer to instance» у TimerQueue.Tick тим самим
    // кадром, яким предмет створився й одразу зник (спавн у повні руки).
    // Деструктор для цього запізно: черга тримає виклик, а не власника.
    //
    // Таймер -- ref-поле предмета: він помирає РАЗОМ із ним і вистрелити по
    // мертвому не може.
    private ref Timer m_SeedTimer;

    override void EEInit()
    {
        super.EEInit();

        if (!GetGame().IsServer())
            return;

        m_SeedTimer = new Timer(CALL_CATEGORY_SYSTEM);
        m_SeedTimer.Run(0.001, this, "OZ_SeedFromProfile", NULL, false);
    }

    override void EEItemAttached(EntityAI item, string slot_name)
    {
        super.EEItemAttached(item, slot_name);

        // Батарея не модуль, і відсіку в неї немає -- але заряд і сама її
        // наявність їдуть на клієнт саме звідси. Без цього вставлена батарея
        // з'являлась би на екрані лише при наступному чужому оновленні.
        if (slot_name == OZ_PdaConst.SLOT_BATTERY && GetGame().IsServer())
            PushState();

        // Біт GPS -- ДО раннього виходу за idx: вигоріла плата міняє відповідь
        // так само, як вийнята, і жоден із цих шляхів не мусить його минути.
        OZ_SyncHardwareBits();

        int idx = SlotIndexOf(slot_name);
        if (idx == -1)
            return;

        m_ModuleAcc[idx] = 0;

        OZ_ModuleBehaviour b = OZ_PdaModules.ForClass(item.GetType());
        if (b)
            b.OnAttached(this, idx);

        // Набір модулів змінився -- період теж міг: детектор на пів секунди
        // поруч із дозиметром на десять означає пів секунди на обох.
        ArmModuleTicks();
    }

    override void EEItemDetached(EntityAI item, string slot_name)
    {
        super.EEItemDetached(item, slot_name);

        if (slot_name == OZ_PdaConst.SLOT_BATTERY && GetGame().IsServer())
            PushState();

        OZ_SyncHardwareBits();

        int idx = SlotIndexOf(slot_name);
        if (idx == -1)
            return;

        // Гасить свої звуки й ефекти мусить сам модуль: КПК за чужим кодом
        // не прибирає й не може знати, що той завів.
        OZ_ModuleBehaviour b = OZ_PdaModules.ForClass(item.GetType());
        if (b)
            b.OnDetached(this, idx);

        // Вийняли останній тікаючий модуль -- таймер СТАЄ, а не крутиться
        // далі вхолосту до вимикання приладу.
        ArmModuleTicks();
    }

    private int SlotIndexOf(string slot_name)
    {
        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            if (OZ_PdaConst.ModuleSlot(i) == slot_name)
                return i;
        }
        return -1;
    }

    // Найменший період серед поведінок прикріплених модулів. Нуль -- нікому
    // тікати. Рахується на attach/detach і на вмиканні, а не на кожному тіку:
    // між цими подіями відповідь не міняється.
    private float SmallestPeriod()
    {
        float best = 0;

        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            string cls = OZ_ModuleClass(i);
            if (cls == "")
                continue;

            OZ_ModuleBehaviour b = OZ_PdaModules.ForClass(cls);
            if (!b)
                continue;

            float p = b.TickSeconds();
            if (p <= 0)
                continue;   // декларативний модуль, як звичайна антена

            if (best <= 0 || p < best)
                best = p;
        }

        return best;
    }

    // Завести таймер під поточний набір модулів -- або зупинити його, коли
    // тікати нема кому. Ідемпотентна: період той самий -- нічого не робимо,
    // інакше кожен attach скидав би відлік уже запущеного тіка.
    private void ArmModuleTicks()
    {
        // Набір плат міг змінитись -- отже могла змінитись і витрата.
        // Одна точка на обидва наслідки: сюди приходять attach, detach,
        // вмикання (OnWorkStart) і прокидання вже ввімкненим (RefreshPower).
        OZ_ApplyDrain();
        // І третій наслідок: те, що клієнт мусить знати про залізо (GPS).
        // Тут -- бо саме сюди сходяться ВСІ причини, з яких набір міг стати
        // іншим, включно з першим прокиданням приладу після завантаження.
        OZ_SyncHardwareBits();

        if (!m_IsOn)
        {
            StopModuleTicks();
            return;
        }

        float want = SmallestPeriod();
        if (want <= 0)
        {
            StopModuleTicks();
            return;
        }

        if (m_ModuleTimer && m_ModuleTimer.IsRunning() && m_TickPeriod == want)
            return;

        if (!m_ModuleTimer)
            m_ModuleTimer = new Timer(CALL_CATEGORY_SYSTEM);

        m_ModuleTimer.Stop();
        for (int a = 0; a < m_ModuleAcc.Count(); a++)
            m_ModuleAcc[a] = 0;

        m_TickPeriod  = want;
        m_LastTickMs  = GetGame().GetTime();
        m_ModuleTimer.Run(m_TickPeriod, this, "ModuleTick", NULL, true);
        OZ_Log.Dbg("pda module tick armed at " + m_TickPeriod.ToString() + "s on " + GetType());
    }

    // ВИТРАТА = БАЗА ПРОФІЛЮ x ДОБУТОК PowerFactor ВСТАВЛЕНИХ ПЛАТ
    // (ТЗ-5 R-B2.2). До цієї правки витрата була одним плоским числом у
    // config.cpp, а PowerFactor у Hardware.json -- позначкою, яку не читав
    // ніхто: адмін піднімав множник радіометра до двійки й не бачив у грі
    // нічого. Тепер важіль працює в обидва боки, і тир приладу нарешті
    // означає щось у батареї, а не лише в наборі сторінок.
    //
    // Рушій зберігає витрату ЗА СЕКУНДУ (componentenergymanager.c:1843:
    // consume_energy = GetEnergyUsage() * секунди), профіль оголошує її за
    // хвилину -- ділення на 60 стоїть тут, в одному місці.
    //
    // OZ_ModuleClass, а не сирий обхід гнізд: він уже знімає з рахунку
    // плату у СХОВАНОМУ відсіку (ТЗ-4 R-F2.1) і ВИГОРІЛУ плату
    // (IsRuined) -- і те, і те не працює, отже й не їсть.
    // ЩО КЛІЄНТ МУСИТЬ ЗНАТИ ПРО ЗАЛІЗО, а спитати не може.
    //
    // Сьогодні це рівно одне питання: чи знає прилад, де він (ТЗ-4 R-B2.2).
    // Худ малює мінікарту з НАДІТОГО приладу (задача 62), сервера при цьому
    // не питає взагалі -- і питати не мусить, це оверлей на кожному кадрі.
    // OZ_HasModuleKind йому не відповість: таблиця `Kind` серверна (§10 D82).
    //
    // Дорожчого конвеєра тут не треба: біт міняється лише коли міняється
    // набір плат, а SetSynchDirty кличемо лише коли він СПРАВДІ інший --
    // інакше кожне під'єднання батареї слало б пакет ні про що.
    void OZ_SyncHardwareBits()
    {
        if (!GetGame().IsServer())
            return;

        bool gps = OZ_HasModuleKind(OZ_PdaConst.MOD_GPS);
        if (gps == m_HasGpsS)
            return;

        m_HasGpsS = gps;
        SetSynchDirty();
    }

    // Чи стоїть у приладі робочий приймач GPS. Читається з ОБОХ боків:
    // сервер міряє, клієнт читає дзеркало.
    bool OZ_HasGps()
    {
        return m_HasGpsS;
    }

    void OZ_ApplyDrain()
    {
        if (!GetGame().IsServer())
            return;
        if (!HasEnergyManager())
            return;

        float perMin = OZ_PdaConst.DRAIN_DEFAULT;
        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(GetType());
        if (prof && prof.PowerDrainPerMin > 0)
            perMin = prof.PowerDrainPerMin;

        float factor = 1.0;
        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            string cls = OZ_ModuleClass(i);
            if (cls == "")
                continue;

            OZ_ModuleSpec spec = OZ_PdaHardware.ModuleFor(cls);
            if (!spec)
                continue;
            if (spec.PowerFactor <= 0)
                continue;

            factor = factor * spec.PowerFactor;
        }

        float perSec = (perMin * factor) / 60.0;
        GetCompEM().SetEnergyUsage(perSec);

        string dl = "pda drain: " + GetType();
        dl += " base=" + perMin.ToString();
        dl += "/min factor=" + factor.ToString();
        dl += " -> " + (perMin * factor).ToString() + "/min";
        OZ_Log.Dbg(dl);
    }

    private void StopModuleTicks()
    {
        if (m_ModuleTimer)
            m_ModuleTimer.Stop();
        m_TickPeriod = 0;
    }

    void ModuleTick()
    {
        // Вимкнений пристрій нічого не міряє й не пищить.
        if (!m_IsOn)
        {
            StopModuleTicks();
            return;
        }

        // ЧАС МІРЯЄМО ГОДИННИКОМ, А НЕ ПЕРІОДОМ ТАЙМЕРА.
        //
        // Накопичувач додавав ОГОЛОШЕНИЙ період, тобто вірив, що таймер
        // спрацьовує рівно за розкладом. Поки період був спільною чвертю
        // секунди, похибка ділилась на двадцять витків; тепер таймер біжить
        // періодом самої поведінки, і кожен його зсув -- просадка сервера,
        // затримка черги -- одразу стає похибкою В РЕСУРСІ, який поведінка
        // списує (SpyMinutes шпигунської плати рахується саме цим dt).
        //
        // Годинник рушія такої помилки не має. Виміряно на стенді
        // 2026-09-06: таймер на 5 с дає 0,199 виклику на секунду, тобто 5,03 с
        // між витками -- саме ці 30 мс і не мусять накопичуватись.
        int nowMs = GetGame().GetTime();
        float elapsed = (nowMs - m_LastTickMs) / 1000.0;
        m_LastTickMs = nowMs;
        // Перший виток після заведення й перескок годинника -- беремо період.
        if (elapsed <= 0 || elapsed > 60)
            elapsed = m_TickPeriod;

        Man owner = Man.Cast(GetHierarchyRootPlayer());

        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            string cls = OZ_ModuleClass(i);
            if (cls == "")
                continue;

            OZ_ModuleBehaviour b = OZ_PdaModules.ForClass(cls);
            if (!b)
                continue;

            float period = b.TickSeconds();
            if (period <= 0)
                continue;   // декларативний модуль, як антена

            m_ModuleAcc[i] = m_ModuleAcc[i] + elapsed;
            if (m_ModuleAcc[i] < period)
                continue;

            float dt = m_ModuleAcc[i];
            m_ModuleAcc[i] = 0;
            b.OnTick(this, owner, dt);
        }
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();

        // Клієнт дізнається про вмикання лише звідси -- OnWorkStart до нього
        // не доходить.
        if (m_IsOn)
            ArmModuleTicks();
        else
            StopModuleTicks();
    }

    bool  OZ_IsOn()      { return m_IsOn; }
    float OZ_Charge01()  { return m_Charge01; }
    bool  OZ_HasPin()    { return m_Pin != ""; }
    bool  OZ_AutoLock()  { return m_AutoLock; }
    string OZ_SessionUid() { return m_SessionUid; }

    // ---------------------------------------------------------------- замок

    // Автоблокування рахується ЛІНИВО, у момент звернення, а не таймером.
    // Причина проста: вимкнений пристрій у рюкзаку не тікає, і будити заради
    // нього тік -- марна робота на кожному КПК на сервері.
    void OZ_EvaluateLock(float lockAfterMinutes)
    {
        if (!GetGame().IsServer())
            return;

        if (!m_Unlocked || !m_AutoLock || m_Pin == "")
            return;

        if (m_LeftHandsAt == 0)   // досі в руках -- відлік не почався
            return;

        if (lockAfterMinutes <= 0)
            return;

        int elapsedMs = GetGame().GetTime() - m_LeftHandsAt;
        if (elapsedMs >= lockAfterMinutes * 60000)
        {
            m_Unlocked = false;
            SetSynchDirty();
            OZ_Log.Dbg("pda locked itself after being put away");
        }
    }

    // КЛІЄНТСЬКА правда про замок: обидва біти синхронні, тож худ і умови
    // дій можуть питати без RPC. Замкнений -- значить пін Є і не введений.
    bool OZ_LockedForViewer()
    {
        return m_HasPinS && !m_Unlocked;
    }

    bool OZ_IsUnlocked()
    {
        if (m_Pin == "")
            return true;
        return m_Unlocked;
    }

    // Повертає true, якщо код збігся. Лічильник невдач росте тільки тут.
    bool OZ_TryUnlock(string uid, string attempt)
    {
        if (!GetGame().IsServer())
            return false;

        // Коду немає -- відмикається завжди, але ВІДМИКАЄТЬСЯ, а не просто
        // відповідає «так». Після рестарту пристрій приходить замкненим
        // незалежно від того, є на ньому код чи ні, і без цього рядка КПК
        // без піна лишався б замкненим назавжди.
        if (m_Pin == "")
        {
            m_Unlocked = true;
            SetSynchDirty();
            return true;
        }

        if (OZ_IsLockedOut(uid))
            return false;

        if (attempt != m_Pin)
        {
            BumpFails(uid);
            return false;
        }

        m_Unlocked = true;
        SetSynchDirty();
        ResetFails(uid);
        return true;
    }

    void OZ_Lock()
    {
        if (!GetGame().IsServer())
            return;
        m_Unlocked = false;
        SetSynchDirty();
    }

    bool OZ_IsLockedOut(string uid)
    {
        if (OZ_FailsFor(uid) < OZ_PdaTune.PinMaxFails())
            return false;

        // Вікно блокування: сплило -- лічильник прощається сам. Нуль у
        // конфізі означає стару поведінку «до рестарту сервера».
        int windowMs = OZ_PdaTune.PinLockoutMs();
        if (windowMs <= 0)
            return true;

        int i = m_FailUid.Find(uid);
        if (i == -1)
            return false;

        if (GetGame().GetTime() - m_FailAt[i] >= windowMs)
        {
            ResetFails(uid);
            return false;
        }
        return true;
    }

    // Скільки секунд лишилось до кінця блокування. 0 -- не заблоковано або
    // блок до рестарту (тоді чесно нема чого рахувати).
    int OZ_LockWaitSec(string uid)
    {
        if (!OZ_IsLockedOut(uid))
            return 0;

        int windowMs = OZ_PdaTune.PinLockoutMs();
        if (windowMs <= 0)
            return 0;

        int i = m_FailUid.Find(uid);
        if (i == -1)
            return 0;

        int left = m_FailAt[i] + windowMs - GetGame().GetTime();
        if (left < 0)
            left = 0;
        return left / 1000;
    }

    int OZ_FailsFor(string uid)
    {
        int i = m_FailUid.Find(uid);
        if (i == -1)
            return 0;
        return m_FailCount[i];
    }

    // ------------------------------------------------------ запечатаний КПК

    bool OZ_IsSealed()
    {
        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(GetType());
        if (!prof || !prof.Sealed)
            return false;

        // Профіль каже «запечатаний», але зламаний пристрій уже не такий.
        // Ознака зламаності -- відсутність коду: код у нього ставив сервер, і
        // прибрати його могло лише зламування.
        // З 2026-09-02 ознака зламаності -- окремий прапорець (ТЗ-4 R-B3.2):
        // власний код, поставлений після зламу, печаткою не є.
        return m_Pin != "" && !m_Cracked;
    }

    bool OZ_IsCracking()
    {
        return m_CrackUntil > 0;
    }

    int OZ_CrackLeftSec()
    {
        if (m_CrackUntil <= 0)
            return 0;

        int left = m_CrackUntil - GetGame().GetTime();
        if (left <= 0)
            return 0;
        return left / 1000;
    }

    bool OZ_HasDecryptor()
    {
        return OZ_HasModuleKind(OZ_PdaConst.MOD_DECRYPTOR);
    }

    // Почати злам. Причину відмови повертаємо рядком: «нема чого ламати» й
    // «нема чим» -- різні біди.
    string OZ_StartCrack(float seconds)
    {
        if (!GetGame().IsServer())
            return "STR_OZ_ERR_PDA_INTERNAL";

        // Ламається ЗАМОК, а не тільки печатка (рішення власника
        // 2026-08-28): звичайний замкнений пін дешифратор бере так само.
        // Мусить бути ЩО ламати: без піна чи на відімкненому нема замка.
        if (!OZ_IsSealed())
        {
            if (m_Pin == "")
                return "STR_OZ_ERR_NO_PIN";
            if (m_Unlocked)
                return "STR_OZ_ERR_NOT_LOCKED";
        }

        if (!OZ_HasDecryptor())
            return "STR_OZ_ERR_NO_DECRYPTOR";

        if (m_CrackUntil > 0)
            return "STR_OZ_ERR_BUSY";

        int ms = Math.Round(seconds * 1000);
        if (ms < 1000)
            ms = 1000;

        m_CrackUntil = GetGame().GetTime() + ms;
        return "";
    }

    // Ліниво, як і замок: будити тік заради кожного запечатаного КПК на
    // сервері марно, а спитають про нього рівно тоді, коли на нього дивляться.
    void OZ_EvaluateCrack()
    {
        if (!GetGame().IsServer())
            return;
        if (m_CrackUntil <= 0)
            return;

        // Дешифратор мусить бути на місці -- і В ЦЮ МИТЬ, а не лише поки
        // хтось дивиться на відлік.
        //
        // Перевірка стояла ТІЛЬКИ в гілці «ще не добігло», і цього вистачало
        // рівно доти, доки на прилад дивляться. Закрий меню, вийми модуль,
        // переклади в другий запечатаний КПК, почни другий злам -- і обидва
        // добіжать: перериванню не було коли спрацювати, а завершення модуля
        // не питало зовсім. Один дешифратор відкривав скільки завгодно
        // приладів паралельно -- саме те, що комент нижче обіцяв не пускати.
        //
        // Правило тепер одне й перевіряється ліниво, без жодного тіка:
        // дешифратор мусить бути в приладі тієї миті, коли злам ЗАКІНЧУЄТЬСЯ.
        if (!OZ_HasDecryptor())
        {
            m_CrackUntil = 0;
            OZ_Log.Dbg("crack aborted: decryptor removed");
            return;
        }

        if (GetGame().GetTime() < m_CrackUntil)
            return;

        m_CrackUntil = 0;
        m_Pin        = "";
        m_HasPinS    = false;
        m_Unlocked   = true;
        m_Cracked    = true;
        SetSynchDirty();

        // Дешифратор ОДНОРАЗОВИЙ: успішний злам спалює плату (рішення
        // власника 2026-08-28). Руїна лишається в гнізді хламом -- чесний
        // слід того, чим пристрій відкривали.
        for (int di = 0; di < OZ_PdaConst.MODULE_SLOTS_MAX; di++)
        {
            string dcls = OZ_ModuleClass(di);
            if (dcls == "")
                continue;

            OZ_ModuleSpec dspec = OZ_PdaHardware.ModuleFor(dcls);
            if (!dspec || dspec.Kind != OZ_PdaConst.MOD_DECRYPTOR)
                continue;

            ItemBase burnt = ItemBase.Cast(OZ_Attached(OZ_PdaConst.ModuleSlot(di)));
            if (burnt)
            {
                burnt.SetHealth("", "", 0);
                OZ_Log.Info("pda: decryptor burnt out on " + GetType());
            }
            break;
        }

        OZ_Log.Dbg("pda cracked open: " + GetType());
    }

    // Записати на пристрій те, що профіль велів -- РІВНО ОДИН раз за життя
    // предмета. Кличе таймер, заведений у EEInit сервера, наступним кадром
    // (див. m_SeedTimer про те, чому не в самому EEInit), і заводський
    // скид -- напряму. Метод НЕ приватний: Timer шукає його по імені.
    void OZ_SeedFromProfile()
    {
        if (!GetGame().IsServer() || m_Seeded)
            return;

        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(GetType());
        if (!prof)
            return;

        m_Seeded = true;

        if (prof.PresetMarkers && prof.PresetMarkers.Count() > 0)
        {
            OZ_MarkerList list = new OZ_MarkerList();
            for (int i = 0; i < prof.PresetMarkers.Count(); i++)
            {
                OZ_MapMarker src = prof.PresetMarkers[i];

                OZ_MapMarker m = new OZ_MapMarker();
                m.Id   = "preset#" + i.ToString();
                m.Name = src.Name;
                m.Pos  = src.Pos;
                list.Items.Insert(m);
            }

            string json;
            string err;
            if (JsonFileLoader<OZ_MarkerList>.MakeData(list, json, err, false))
            {
                m_MarkersJson = json;
                m_OwnCells    = -1;
            }
        }

        if (prof.Sealed)
        {
            // Код ставить СЕРВЕР і не каже його нікому -- зокрема й собі в
            // лог. Підібрати його не можна не тому, що він складний, а тому
            // що його не існує в жодній голові.
            m_Pin = "";
            int pinLen = OZ_PinLength();
            for (int pd = 0; pd < pinLen; pd++)
                m_Pin += Math.RandomInt(0, 10).ToString();

            m_HasPinS  = true;
            m_Unlocked = false;
            m_AutoLock = true;
            SetSynchDirty();
        }
    }

    // --------------------------------------------------------------- мітки

    string OZ_MarkersJson()
    {
        return m_MarkersJson;
    }

    void OZ_SetMarkersJson(string json)
    {
        if (!GetGame().IsServer())
            return;
        m_MarkersJson = json;
        m_OwnCells = -1;
    }

    // ------------------------------------------------------------- записки

    string OZ_NotesJson()
    {
        return m_NotesJson;
    }

    void OZ_SetNotesJson(string json)
    {
        if (!GetGame().IsServer())
            return;
        m_NotesJson = json;
        m_OwnCells = -1;
    }

    // ------------------------------------------------------------- маршрут

    string OZ_RouteJson()
    {
        return m_RouteJson;
    }

    void OZ_SetRouteJson(string json)
    {
        if (!GetGame().IsServer())
            return;
        m_RouteJson = json;
        m_OwnCells = -1;
    }

    // ------------------------------------------------- розділи чужих модулів
    //
    // Те саме сховище, що й у носія, але НА САМОМУ ПРИЛАДІ й з іншою стелею --
    // чому вони різні, сказано в шапці OZ_SectionStore.
    //
    // Модуль, який хоче тримати своє в пристрої, кличе три методи й більше
    // нічого не знає ні про КПК, ні про його сховище. Імена цих трьох не
    // мінялись, і жоден кличучий не помітив злиття.

    string OZ_KindRead(string kind)
    {
        return m_Store.Read(kind);
    }

    // НУЛЬ на відсутній розділ, а не -1, яким відповідає саме сховище: це
    // число йде в OZ_RoomFor доданком до вільного місця, і -1 украло б там
    // одну ячейку в роду, якого на приладі ще немає.
    int OZ_KindRecords(string kind)
    {
        int n = m_Store.Records(kind);
        if (n < 0)
            return 0;
        return n;
    }

    // ------------------------------------------------------- ЯЧЕЙКИ ПАМ'ЯТІ
    //
    // Одна стеля на весь прилад, і ціна однакова: мітка -- ячейка, нотатка --
    // ячейка, частота -- ячейка, маршрут -- ячейка. Чому одна, а не по стелі
    // на рід -- у коментарі до OZ_PdaLimits.
    //
    // Мітки, нотатки й маршрут лишились окремими полями (вони старші за цей
    // механізм і читаються з тих самих позицій сховища), але РАХУЮТЬСЯ
    // РАЗОМ із чужими розділами: пам'ять одна, і байдуже, хто її зайняв.

    // Число -- з профілю, і лише з нього (ТЗ-4 R-F1.2); Validate уже
    // гарантує, що воно там є. Без профілю (клас, якого немає в
    // Profiles.json) -- те саме задокументоване умовчання.
    int OZ_Max()
    {
        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(GetType());
        if (prof && prof.Limits && prof.Limits.Memory > 0)
            return prof.Limits.Memory;
        return OZ_PdaConst.MEMORY_DEFAULT;
    }

    // Скільки ячеек коштують ВЛАСНІ розділи приладу. Рахуємо розбором, а не
    // лічильником поруч: лічильник, який хтось забув оновити, бреше мовчки,
    // а розбір не може розійтися з тим, що справді записано.
    private int OwnCells()
    {
        // Порахували раніше й нічого відтоді не міняли -- віддаємо готове.
        if (m_OwnCells >= 0)
            return m_OwnCells;

        int n = 0;

        if (m_MarkersJson != "")
        {
            OZ_MarkerList marks = new OZ_MarkerList();
            string e1;
            if (JsonFileLoader<OZ_MarkerList>.LoadData(m_MarkersJson, marks, e1) && marks && marks.Items)
                n += marks.Items.Count();
        }

        if (m_NotesJson != "")
        {
            OZ_NoteBook book = new OZ_NoteBook();
            string e2;
            if (JsonFileLoader<OZ_NoteBook>.LoadData(m_NotesJson, book, e2) && book && book.Notes)
                n += book.Notes.Count();
        }

        // Маршрут -- ОДНА ячейка незалежно від довжини: він лише порядок
        // зв'язків між мітками, а самі мітки вже пораховані вище. Тому
        // перенести маршрут -- це перенести його мітки ПЛЮС його самого.
        if (m_RouteJson != "")
            n += 1;

        m_OwnCells = n;
        return n;
    }

    int OZ_Used()
    {
        return OwnCells() + m_Store.Used();
    }

    int OZ_Free()
    {
        int free = OZ_Max() - OZ_Used();
        if (free < 0)
            return 0;
        return free;
    }

    // Скільки ячеек цей рід може зайняти ВСЬОГО: вільні плюс ті, що він
    // тримає зараз. Інакше перезапис свого ж розділу рахувався б двічі.
    int OZ_RoomFor(string kind)
    {
        return OZ_Free() + OZ_KindRecords(kind);
    }

    bool OZ_KindWrite(string kind, string json, int records)
    {
        if (!GetGame().IsServer() || kind == "")
            return false;

        if (records < 0)
            records = 0;

        int room = OZ_RoomFor(kind);
        if (records > room)
        {
            string full = "pda memory: " + kind + " asks " + records.ToString();
            full += " cell(s), room for " + room.ToString();
            OZ_Log.Warn(full);
            return false;
        }

        // Порожній запис -- це стирання розділу, а не розділ із порожнім
        // текстом: інакше прилад накопичував би мертві роди.
        if (json == "")
        {
            m_Store.Drop(kind);
            return true;
        }

        m_Store.Put(kind, json, records);
        return true;
    }

    // --------------------------------------------------------------- сесія

    bool OZ_HasSession(string uid, int playerEpoch)
    {
        if (m_SessionUid == "")
            return false;
        if (m_SessionUid != uid)
            return false;
        // Епоха розійшлась -- гравець скинув сесії з іншого пристрою.
        return m_SessionEpoch == playerEpoch;
    }

    // Чи є на пристрої ХОЧ ЯКАСЬ сесія -- незалежно від того, чия й чи жива.
    // Питання окреме від OZ_HasSession навмисно: «сесії немає взагалі» і
    // «сесія є, але чужа» -- різні стани, і плутати їх означало б віддавати
    // чужий пристрій новому власнику з одного дотику.
    bool OZ_HasAnySession()
    {
        return m_SessionUid != "";
    }

    void OZ_OpenSession(string uid, int playerEpoch)
    {
        if (!GetGame().IsServer())
            return;
        m_SessionUid   = uid;
        m_SessionEpoch = playerEpoch;
    }

    // -------------------------------------------------------------- знімок

    // Онлайн -- це коли епоха пристрою збігається з епохою гравця. Розійшлись
    // -- пристрій живий, читається, але більше не оновлюється.
    bool OZ_IsOnline(int playerEpoch)
    {
        if (m_SessionUid == "")
            return false;
        return m_SessionEpoch == playerEpoch;
    }

    string OZ_Snapshot()   { return m_Snapshot; }
    string OZ_SnapshotAt() { return m_SnapshotAt; }

    // ПІДПИС ЗНІМКА -- те, з чого його зібрали минулого разу.
    //
    // Не зберігається: після рестарту перший же статус перезбере знімок, і
    // це дешевше, ніж тягнути рядок крізь сховище.
    private string m_SnapSig = "";

    string OZ_SnapshotSig() { return m_SnapSig; }

    // ШТАМП -- ОКРЕМО ВІД ТІЛА, і це не дрібниця.
    //
    // m_SnapshotAt -- мить, по якій ріжеться історія розмов, коли пристрій
    // стане капсулою. Він мусить іти вперед на КОЖНОМУ живому статусі, інакше
    // капсула замерзне на моменті останньої зміни складу друзів, і все, що
    // власник наговорив після неї, не побачить ніхто.
    //
    // А от ТІЛО -- ім'я, фракція, база, імена контактів -- міняється раз на
    // кілька годин, і перезбирати його разом із проходом по всіх друзях і
    // серіалізацією кожні п'ять секунд на кожному живому приладі не було
    // потреби ніколи.
    void OZ_TouchSnapshot(int playerEpoch)
    {
        if (!GetGame().IsServer())
            return;
        if (!OZ_IsOnline(playerEpoch))
            return;

        m_SnapshotAt = OZ_Time.NowUtc();
    }

    // Пише лише сервер і лише поки пристрій онлайн.
    void OZ_RefreshSnapshot(int playerEpoch, string json, string sig)
    {
        if (!GetGame().IsServer())
            return;
        if (!OZ_IsOnline(playerEpoch))
            return;

        m_SnapSig    = sig;
        m_Snapshot   = json;
        m_SnapshotAt = OZ_Time.NowUtc();
    }

    // «До заводських» БЕЗ піна: знайдений чужий КПК можна зробити своїм,
    // але ціна чесна -- всі дані попереднього власника згорають. Sealed
    // сюди не пускаємо: запечатане або ламають дешифратором, або носять
    // як цеглину. Чип у гнізді -- фізичний носій, його скидання не чіпає.
    string OZ_FactoryReset()
    {
        if (!GetGame().IsServer())
            return "STR_OZ_ERR_PDA_INTERNAL";

        // Квестовий прилад до заводських не скидається -- ні запечатаний, ні
        // зламаний (ТЗ-4 R-B3.1): скидання посіяло б новий код, якого немає в
        // жодній голові, і прилад ставав би цеглиною.
        OZ_PdaProfile qprof = OZ_PdaProfiles.ForClass(GetType());
        if (qprof && qprof.Sealed)
            return "STR_OZ_ERR_QUEST_RESET";

        if (OZ_IsSealed())
            return "STR_OZ_ERR_SEALED";

        m_Pin      = "";
        m_HasPinS  = false;
        m_Unlocked = true;
        m_AutoLock = true;

        m_SessionUid   = "";
        m_SessionEpoch = 0;
        m_Snapshot     = "";
        m_SnapshotAt   = "";
        m_SnapSig      = "";

        m_MarkersJson = "";
        m_NotesJson   = "";
        m_RouteJson   = "";
        m_Store.Clear();
        m_OwnCells    = -1;
        m_CrackUntil  = 0;

        // ВСІ ТРИ, а не два з трьох.
        //
        // Масиви паралельні: один і той самий індекс означає uid, лічильник і
        // час. Очищення двох лишало третій довшим, і перший же новий промах
        // після скидання зсовував їх назавжди -- m_FailAt[i] починав читати
        // час ЧУЖОГО запису, тобто вікно блокування питали не в того.
        m_FailUid.Clear();
        m_FailCount.Clear();
        m_FailAt.Clear();

        // Заводські пресети -- заново: скинутий профільний прилад знову
        // несе свою фабричну начинку.
        m_Seeded = false;
        OZ_SeedFromProfile();

        SetSynchDirty();
        OZ_Log.Info("pda: factory reset of " + GetType());
        return "";
    }

    // Щоб змінити пін, його треба ЗНАТИ. Сесія тут ні до чого: пристрій не
    // питає, хто ти, він питає старий код. Немає коду -- задати новий може
    // будь-хто, у кого пристрій у руках.
    //
    // Невдала спроба рахується так само, як невдале відмикання: інакше
    // «зміна піна» стала б обхідним шляхом для підбору.
    // ФОРМА КОДУ ПЕРЕВІРЯЄТЬСЯ НА СЕРВЕРІ, а не самою лише клавіатурою.
    //
    // Пад малює рівно чотири цифри, і чесний клієнт інакше й не пошле. Але
    // код приїжджає рядком у RPC, і сервер брав його ЯК Є: підроблений запит
    // ставив на прилад пін завдовжки в мегабайт або з будь-яких знаків, і
    // після цього пад відкрити його не міг НІКОЛИ -- ані власник, ані злодій.
    // Тобто це був однобічний спосіб зробити чужий прилад цеглиною.
    //
    // Порожній рядок лишається законним: це «зняти код».
    private bool PinShaped(string pin)
    {
        if (pin == "")
            return true;

        int want = OZ_PinLength();
        if (pin.Length() != want)
            return false;

        for (int i = 0; i < want; i++)
        {
            int c = pin.Get(i).ToAscii();
            if (c < 48 || c > 57)   // '0'..'9'
                return false;
        }
        return true;
    }

    // СКІЛЬКИ ЦИФР У КОДІ -- ПИТАННЯ ПРОФІЛЮ (ТЗ-5 R-B3.3).
    //
    // Число одне на три місця: сервер перевіряє ним форму коду, він же сіє
    // код запечатаного приладу, і воно ж їде клієнтові в
    // OZ_PdaDeviceStatus.PinLength, щоб пад намалював рівно стільки крапок.
    // Раніше чотири були правилом КЛІЄНТА: пад малював чотири, а сервер
    // приймав будь-який рядок будь-якої довжини.
    //
    // Профіль, якого немає (клас не вписаний у Profiles.json), дає
    // задокументовану четвірку -- те саме число, що стояло константою.
    int OZ_PinLength()
    {
        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(GetType());
        if (prof && prof.PinLength > 0)
            return prof.PinLength;
        return OZ_PdaConst.PIN_LENGTH_DEFAULT;
    }

    bool OZ_SetPin(string uid, string oldPin, string newPin)
    {
        if (!GetGame().IsServer())
            return false;

        if (!PinShaped(newPin))
        {
            OZ_Log.Warn("pda: refused a PIN that is not " + OZ_PinLength().ToString() + " digits from " + uid);
            return false;
        }

        if (m_Pin != "")
        {
            if (OZ_IsLockedOut(uid))
                return false;

            if (oldPin != m_Pin)
            {
                BumpFails(uid);
                return false;
            }
        }

        m_Pin = newPin;
        m_HasPinS = (m_Pin != "");
        m_Unlocked = true;   // не замикаємо того, хто щойно задав код
        SetSynchDirty();
        ResetFails(uid);
        return true;
    }

    // Автоблокування вимикає будь-хто, у кого пристрій відімкнений у руках.
    // Сервер може заборонити його вимикати зовсім -- це рішення адміна, а не
    // гравця.
    bool OZ_SetAutoLock(bool on, bool serverForces)
    {
        if (!GetGame().IsServer())
            return false;
        if (!OZ_IsUnlocked())
            return false;

        if (serverForces && !on)
            return false;

        m_AutoLock = on;
        return true;
    }

    private void BumpFails(string uid)
    {
        int i = m_FailUid.Find(uid);
        if (i == -1)
        {
            m_FailUid.Insert(uid);
            m_FailCount.Insert(1);
            m_FailAt.Insert(GetGame().GetTime());
            return;
        }
        m_FailCount[i] = m_FailCount[i] + 1;
        m_FailAt[i]    = GetGame().GetTime();
    }

    // РЯДОК ЗНИКАЄ ЦІЛКОМ, а не обнуляється.
    //
    // Обнулений лічильник читається так само, як відсутній запис (OZ_FailsFor
    // на невідомому uid віддає нуль), а три паралельні масиви росли назавжди:
    // кожен, хто хоч раз помилився кодом, лишався в предметі до кінця його
    // життя. ВСІ ТРИ й тим самим індексом -- вони паралельні, і зняти два з
    // трьох означало б зсунути час чужого запису (див. OZ_FactoryReset).
    private void ResetFails(string uid)
    {
        int i = m_FailUid.Find(uid);
        if (i == -1)
            return;

        m_FailUid.Remove(i);
        m_FailCount.Remove(i);
        m_FailAt.Remove(i);
    }

    // Відлік автоблокування починається, коли пристрій пішов З РУК.
    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);

        if (!GetGame().IsServer())
            return;

        if (newLoc.GetType() == InventoryLocationType.HANDS)
            m_LeftHandsAt = 0;
        else if (oldLoc.GetType() == InventoryLocationType.HANDS)
            m_LeftHandsAt = GetGame().GetTime();
    }

    // ------------------------------------------------------------- залізо

    EntityAI OZ_Attached(string slotName)
    {
        return OZ_AttachedId(OZ_PdaSlots.Of(slotName));
    }

    // За ГОТОВИМ id -- для тих, хто в гарячому шляху й уже має число.
    EntityAI OZ_AttachedId(int slotId)
    {
        if (slotId == -1)
            return null;
        return GetInventory().FindAttachment(slotId);
    }

    // Плата у відсіку i ФІЗИЧНО -- разом із вигорілою, -- або null.
    //
    // СХОВАНИЙ ВІДСІК -- ВИМКНЕНИЙ ВІДСІК (ТЗ-4 R-F2.1). ModuleSlots профілю
    // досі лише ховав гнізда понад число, а модуль у схованому працював
    // повністю. Усі, хто питає «що стоїть у відсіку», проходять тут, тож
    // одного місця досить: антена, дешифратор, GPS, витрата живлення.
    //
    // Правило живе саме тут, а не в OZ_ModuleClass, бо є питання і про
    // ФІЗИКУ гнізда, а не лише про робочу плату: статус приладу мусить
    // підписати вигорілу (ТЗ-5 R-B2.9), тобто повз відповідь «нічого не
    // працює». Читаючи гніздо повз ці двері, він возив клієнту вміст
    // схованого відсіку -- назву, вид і залишок ресурсу.
    EntityAI OZ_ModuleSeat(int i)
    {
        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(GetType());
        if (prof && i >= prof.ModuleSlots)
            return null;

        return OZ_AttachedId(OZ_PdaSlots.Module(i));
    }

    // Класнейм РОБОЧОГО модуля у відсіку i, або порожній рядок.
    string OZ_ModuleClass(int i)
    {
        EntityAI m = OZ_ModuleSeat(i);
        if (!m)
            return "";

        // Зруйнована плата -- мертва електроніка: гніздо зайняте, а модуля
        // ФУНКЦІОНАЛЬНО немає. Саме так згорілий дешифратор перестає
        // ламати, а спалений замірник -- міряти.
        ItemBase mi = ItemBase.Cast(m);
        if (mi && mi.IsRuined())
            return "";

        return m.GetType();
    }

    // Чи вставлено модуль такого виду. Питаємо ВИД, а не класнейм: КПК не
    // мусить знати, чия саме антена вставлена, щоб зрозуміти, що зв'язок є.
    bool OZ_HasModuleKind(string kind)
    {
        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            string cls = OZ_ModuleClass(i);
            if (cls == "")
                continue;

            OZ_ModuleSpec spec = OZ_PdaHardware.ModuleFor(cls);
            if (spec && spec.Kind == kind)
                return true;
        }
        return false;
    }

    // Ховає відсіки понад те, що дозволяє профіль. Слоти не додаються в
    // рантаймі, тому в конфізі їх максимум, а профіль ріже видиме.
    override bool CanDisplayAttachmentSlot(int slot_id)
    {
        if (!super.CanDisplayAttachmentSlot(slot_id))
            return false;

        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(GetType());
        if (!prof)
            return true;

        for (int i = prof.ModuleSlots; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            if (slot_id == OZ_PdaSlots.Module(i))
                return false;
        }
        return true;
    }

    string OZ_CarrierClass()
    {
        EntityAI c = OZ_AttachedId(OZ_PdaSlots.Carrier());
        if (!c)
            return "";
        return c.GetType();
    }

    // ----------------------------------------------------------- живлення

    // Вмикання/вимикання на прохання інтерфейсу.
    //
    // Той самий важіль, що й ванільні ActionTurnOnWhileInHands /
    // ActionTurnOffWhileInHands, які вже висять у SetActions: пристрій живе
    // без нашого інтерфейсу зовсім -- підняв, вставив батарею, увімкнув.
    // Кнопка на сторінці «Пристрій» смикає той самий CompEM, тільки через
    // сервер, а не повз нього.
    //
    // Відповідає рядком-ПРИЧИНОЮ, а не мовчазним false: «немає батареї» й
    // «пристрій не вміє вмикатись» -- різні речі, і гравцеві треба сказати,
    // котра з них.
    string OZ_SetPower(bool on)
    {
        if (!HasEnergyManager())
            return "STR_OZ_ERR_NO_POWER";

        ComponentEnergyManager em = GetCompEM();

        if (on)
        {
            if (!em.GetEnergySource())
                return "STR_OZ_ERR_NO_BATTERY";
            if (!em.CanSwitchOn())
                return "STR_OZ_ERR_NO_POWER";
            em.SwitchOn();
        }
        else
        {
            em.SwitchOff();
        }

        PushState();
        return "";
    }

    override void OnWork(float consumed_energy)
    {
        super.OnWork(consumed_energy);
        if (GetGame().IsServer())
            PushState();
    }

    override void OnWorkStart()
    {
        super.OnWorkStart();
        if (GetGame().IsServer())
        {
            PushState();
            ArmModuleTicks();
        }
    }

    override void OnWorkStop()
    {
        super.OnWorkStop();
        if (GetGame().IsServer())
        {
            LogPowerLoss();
            PushState();
            StopModuleTicks();
        }
    }

    // ЧОМУ ПРИЛАД СТАВ. Один рядок рівнем Dbg.
    //
    // Рушій гасить пристрій із кількох різних причин -- сіла батарея, предмет
    // зруйновано, предмет намок, живлення вимкнули, -- і робить це мовчки.
    // Гравець і адмін бачать однакове POWERED DOWN, а в лозі не було нічого:
    // 2026-09-01 на з'ясуванні того, що батарея просто порожня, пішла година
    // (D131). Рядок нижче відповідає на це питання одразу.
    private void LogPowerLoss()
    {
        ComponentEnergyManager em = GetCompEM();
        if (!em)
        {
            OZ_Log.Dbg("pda power lost: no energy manager at all");
            return;
        }

        string line = "pda power lost: " + GetType();
        line += " switchedOn=" + em.IsSwitchedOn();
        line += " ruined=" + IsRuined();
        line += " wet=" + GetWet().ToString();

        EntityAI b = OZ_Battery();
        if (b && b.HasEnergyManager())
        {
            line += " batt=" + b.GetCompEM().GetEnergy().ToString();
            line += "/" + b.GetCompEM().GetEnergyMax().ToString();
        }
        else
        {
            line += " batt=NONE";
        }

        OZ_Log.Dbg(line);
    }

    // Батарея у своєму гнізді. Питати про неї треба САМЕ так, а не через
    // GetEnergySource(): джерело з'являється лише коли пристрій увімкнений,
    // тож вимкнений КПК із повною батареєю відповідав би «батареї немає».
    EntityAI OZ_Battery()
    {
        return OZ_AttachedId(OZ_PdaSlots.Battery());
    }

    bool OZ_HasBattery()
    {
        return OZ_Battery() != null;
    }

    // Список дозволених батарей -- у профілі (BatteryClassNames), і досі він
    // був мертвим полем: обіцяв адмінові контроль, якого ніхто не перевіряв.
    // Порожній список означає «влазить усе, що сідає в слот» -- модові
    // батареї з іншою ємністю працюють одразу, бо відсоток рахується від
    // GetEnergyMax() ВСТАВЛЕНОЇ батареї, а не від константи.
    //
    // Лише сервер: у клієнта профілів немає, і його відмова була б
    // ворожінням. Серверна відмова повертає предмет чесно й сама.
    override bool CanReceiveAttachment(EntityAI attachment, int slotId)
    {
        if (!super.CanReceiveAttachment(attachment, slotId))
            return false;

        if (!GetGame().IsServer())
            return true;

        if (slotId != OZ_PdaSlots.Battery())
            return true;

        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(GetType());
        if (!prof || !prof.BatteryClassNames || prof.BatteryClassNames.Count() == 0)
            return true;

        return prof.BatteryClassNames.Find(attachment.GetType()) != -1;
    }

    // Заряд беремо з БАТАРЕЇ, а не з себе: energyStorageMax=0, свого запасу
    // пристрій не має й не повинен.
    private void PushState()
    {
        // Заразом звіряємо біт заліза: набір плат міг стати іншим ще до того,
        // як таблиця заліза встигла завантажитись (порядок модулів CF не
        // гарантований), а сюди прилад приходить на кожне вмикання й на кожну
        // зміну батареї.
        OZ_SyncHardwareBits();

        bool wasOn = m_IsOn;

        m_IsOn = false;
        m_Charge01 = 0;

        if (HasEnergyManager())
            m_IsOn = GetCompEM().IsWorking();

        EntityAI batt = OZ_Battery();
        if (batt && batt.HasEnergyManager())
        {
            float maxE = batt.GetCompEM().GetEnergyMax();
            if (maxE > 0)
                m_Charge01 = batt.GetCompEM().GetEnergy() / maxE;
        }

        // Втратив живлення посеред злому -- злам ПЕРЕРВАНО, не «на паузі».
        // Дешифратор рахує безперервно за годинником гри, а не тіками
        // модуля (PowerFactor 2.0 у Hardware.json описує його апетит до
        // батареї, а не швидкість злому); лінива OZ_EvaluateCrack
        // ловить лише стан «у цю мить» і проґавила б вимкнення між стартом і
        // поглядом -- гравець вимкнув, зачекав без батареї, увімкнув і
        // відкрив за нуль енергії. Перериваємо саме тут, ПОДІЄЮ втрати
        // живлення, а не наступним поглядом.
        if (wasOn && !m_IsOn && m_CrackUntil > 0 && GetGame().IsServer())
        {
            m_CrackUntil = 0;
            OZ_Log.Dbg("crack aborted: device lost power mid-crack");
        }

        // ПРОКИНУВСЯ ВЖЕ УВІМКНЕНИМ -- теж привід завести тік.
        //
        // Тік заводив лише OnWorkStart, а рушій його при завантаженні не
        // кличе: пристрій, який пережив рестарт сервера ввімкненим, приходить
        // назад працюючим через OnWork, і жоден його модуль не тікав до
        // найближчого вимикання й вмикання руками. Перехід «було вимкнено ->
        // стало ввімкнено» ловиться саме тут, і він накриває обидва шляхи.
        if (!wasOn && m_IsOn && GetGame().IsServer())
            ArmModuleTicks();

        SetSynchDirty();
    }

    // ----------------------------------------------------- персистентність
    //
    // ДОПИСУВАТИ тільки в кінець і читати за GetVersion(). Записи CF
    // позиційні: вставка поля в середину зсуває потік і з'їдає дані всіх, хто
    // пише після нас.
    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        auto ctx = storage["OpenZone_PDA"];
        if (!ctx)
            return;

        ctx.Write(m_Pin);
        ctx.Write(m_AutoLock);
        ctx.Write(m_SessionUid);
        ctx.Write(m_SessionEpoch);
        // Знімок і мітки -- шматками: JSON сорока міток чи книжки давно за
        // 1023 байти, а рядок сховища понад цю межу зносить ВЕСЬ блоб
        // предмета при завантаженні -- разом із піном (зміряно зондом;
        // подробиці в OZ_StoreBig). Сумісність зі старим одно-рядковим
        // форматом читач OZ_StoreBig тримає сам.
        OZ_StoreBig.Write(ctx, m_Snapshot);
        ctx.Write(m_SnapshotAt);
        // v2 і далі -- ДОПИСУЄТЬСЯ В КІНЕЦЬ. Вставка в середину зсунула б
        // потік і зробила б нечитними всі старіші збереження.
        OZ_StoreBig.Write(ctx, m_MarkersJson);
        // v3 -- знову В КІНЕЦЬ.
        ctx.Write(m_Seeded);
        // v4 -- знову В КІНЕЦЬ. Книжка велика, тому шматками, як мітки.
        OZ_StoreBig.Write(ctx, m_NotesJson);
        // v5 -- знову В КІНЕЦЬ.
        OZ_StoreBig.Write(ctx, m_RouteJson);
        // v6 -- знову В КІНЕЦЬ. Розділи чужих модулів (частоти рації й що
        // завгодно далі), і живуть вони тут, а не на носії, з тієї ж причини,
        // що нотатки й маршрут: носій -- знімне, чим переносять, а не те, де
        // приладом користуються.
        //
        // ОКРЕМИМИ ЗАПИСАМИ, як у носія, а не одним JSON-документом -- і
        // ТИМ САМИМ кодом, що в носія: порядок полів тут описано один раз,
        // в OZ_SectionStore.WriteTo.
        m_Store.WriteTo(ctx);

        // v7 -- знову В КІНЕЦЬ: зламаність квестового приладу (ТЗ-4 R-B3.2).
        ctx.Write(m_Cracked);
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        auto ctx = storage["OpenZone_PDA"];
        if (!ctx)
            return true;

        if (!ctx.Read(m_Pin))
            return false;
        if (!ctx.Read(m_AutoLock))
            return false;
        if (!ctx.Read(m_SessionUid))
            return false;
        if (!ctx.Read(m_SessionEpoch))
            return false;
        if (!OZ_StoreBig.Read(ctx, m_Snapshot))
            return false;
        if (!ctx.Read(m_SnapshotAt))
            return false;

        // Мітки з'явились у v2. Старіше збереження їх просто не має, і
        // читати звідти нічого -- інакше ми зчитали б чужі байти.
        if (ctx.GetVersion() >= 2)
        {
            if (!OZ_StoreBig.Read(ctx, m_MarkersJson))
                return false;
        }

        if (ctx.GetVersion() >= 3)
        {
            if (!ctx.Read(m_Seeded))
                return false;
        }

        if (ctx.GetVersion() >= 4)
        {
            if (!OZ_StoreBig.Read(ctx, m_NotesJson))
                return false;
        }

        if (ctx.GetVersion() >= 5)
        {
            if (!OZ_StoreBig.Read(ctx, m_RouteJson))
                return false;
        }

        // Розділи з'явились у v6, і саме це число сховище й гейтить: старіше
        // збереження на цій позиції тримає чужі байти.
        if (!m_Store.ReadFrom(ctx, 6))
            return false;

        if (ctx.GetVersion() >= 7)
        {
            if (!ctx.Read(m_Cracked))
                return false;
        }

        // Замок після рестарту закритий: стан «відімкнено» навмисно не
        // зберігається. Пристрій, що пролежав у схроні через рестарт, має
        // питати код.
        m_Unlocked = false;
        m_LeftHandsAt = 0;
        m_HasPinS = (m_Pin != "");

        // Документи щойно приїхали зі сховища повз сеттери -- лічильник
        // ячейок перерахувати.
        m_OwnCells = -1;

        SetSynchDirty();

        return true;
    }

    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionTurnOnWhileInHands);
        AddAction(ActionTurnOffWhileInHands);

        // Відкриття -- ДІЄЮ, а не клавішею. Прилад треба взяти в руки й
        // застосувати, як будь-яку річ у Зоні; хоткей робив із нього вкладку
        // браузера, яку видно з рюкзака.
        AddAction(OZ_ActionOpenPda);

        // Обмін контактами -- теж дія, і теж по цілі: наводиш на людину.
        AddAction(OZ_ActionExchangeContacts);
    }
}
