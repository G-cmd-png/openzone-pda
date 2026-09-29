// Меню КПК: корпус пристрою поверх приглушеного світу.
//
// Стрічка вкладок будується не з реєстру й не з конфіга, а з ВІДПОВІДІ
// СЕРВЕРА про той пристрій, що в руках. Причина: профілі серверні, і
// вирішувати, які вкладки в тебе є, не має права клієнт. Тому меню
// відкривається порожнім, надсилає device/status і домальовує себе.
//
// Один запит, а не два: сторінка «Пристрій» усе одно питає той самий status,
// і її ж відповідь будує стрічку.

class OZ_PdaMenu : UIScriptedMenu
{
    private Widget m_TabRail;
    // Три комірки смуги стану. Знаходяться раз при відкритті, а не по імені
    // на кожному з двох малювальників щосекунди.
    private TextWidget m_StatusLeft;
    private TextWidget m_StatusMid;
    private TextWidget m_StatusRight;
    private Widget m_PageHost;
    private Widget m_LockPanel;
    private Widget m_InitPanel;
    private ButtonWidget m_BtnClose;

    private ref map<string, ref OZ_PdaPage> m_Pages;
    private ref array<Widget> m_Tabs;
    private string m_Current = "";
    private bool m_Built = false;

    // Якою була КОРЕНЕВА панель ванільного HUD до відкриття КПК, і чи ховали
    // ми її в цьому показі взагалі. Повертаємо САМЕ ЦЕ, а не «увімкнено» --
    // див. OnShow/OnHide.
    private bool m_HudWasShown = true;
    private bool m_HudHid = false;

    private ref Timer m_Refresh;

    // Введений код накопичується тут і НІКУДИ більше: на сервер їде рівно
    // один раз, коли гравець підтвердив. Порівнює його сервер.
    private string m_PinBuffer = "";

    // Екран коду має ТРИ приводи з'явитись, і плутати їх не можна:
    //
    //   ""        нікому не треба, панель схована;
    //   "unlock"  пристрій замкнений -- панель ПРИМУСОВА, скасувати не можна;
    //   "set"     гравець сам попросив задати або змінити код;
    //   "clear"   гравець сам попросив зняти код.
    //
    // Різниця не косметична: примусову панель не можна закрити по Esc, а
    // добровільну -- треба, інакше вийти з неї нема як.
    private string m_PinMode = "";

    // Що сказала ОСТАННЯ відповідь `sealed`. Єдиний читач -- RefreshTick,
    // щоб не просити status у пристрою, який його не віддасть. Хибне
    // значення тут не ламає нічого, а лише повертає зайвий запит на секунду:
    // рішення про доступ ухвалює сервер, і цей прапорець його не стосується.
    private bool m_Sealed = false;

    // ЕКРАН ПРИВ'ЯЗАНИЙ ДО ПРИЛАДУ, НА ЯКОМУ ЙОГО ВІДКРИЛИ.
    //
    // БЕЗ ref: предмет належить світу, а не вікну. Сильне посилання з меню
    // тримало б мертвий КПК живим доти, доки гравець не закриє екран.
    //
    // Раніше меню трималось не за сутність, а за ПРАВИЛО: щосекунди питало
    // OZ_PdaHud.Device() і закривалось, коли той не віддавав нічого. Після
    // перевороту порядку (рішення власника 2026-09-08: спершу руки, потім
    // надітий) цього мало -- прилад, покладений у рюкзак, поступився б місцем
    // НАДІТОМУ, і той самий екран поїхав би далі, розмовляючи вже з іншою
    // річчю. Тепер умова точна: у руках має бути ТОЙ САМИЙ предмет.
    //
    // Порівнюємо ПОКАЗНИКИ, а не розіменовуємо: якщо предмет знищили, у руках
    // буде інше (чи ніщо), збіг не станеться, і екран піде за приладом.
    private OZ_PDA_Base m_Device;

    private int    m_PinStep = 0;
    private string m_PinOld  = "";
    private string m_PinNew  = "";
    private bool   m_HasPin  = false;

    // «НАЗАД» ЧИТАЄТЬСЯ ОПИТУВАННЯМ, А НЕ ПОДІЄЮ КЛАВІШІ.
    //
    // OnKeyPress приходить від віджета, який ТРИМАЄ фокус, і поле вводу
    // забирає клавіатуру собі: до меню Escape звідти не доходить. На карті це
    // ставалось одразу -- рушій сам дає фокус першому багаторядковому полю при
    // побудові (той самий факт тримає OZ_PdaPageNotes), тобто MarkerDesc, --
    // і КПК не закривався клавішею взагалі (звіт власника 2026-09-09, дефект 4).
    // На інших сторінках досить було клацнути в поле.
    //
    // Ваніль вирішує це рівно так: ChatInputMenu (5_mission/gui/chat) сам
    // ставить фокус у EditBox і при цьому закривається з Update(), опитуючи
    // UAUIBack; MissionGameplay.ShowChat вішає ті самі виключення входу
    // {"menu"}, що й ми, -- отже виключення цей вхід не глушать. Заразом
    // Escape перестає бути прибитим: гравець, який перепризначив «назад»,
    // отримує СВОЮ клавішу.
    private UAIDWrapper m_BackInput;

    void OZ_PdaMenu()
    {
        m_Pages = new map<string, ref OZ_PdaPage>();
        m_Tabs  = new array<Widget>();
    }

    override Widget Init()
    {
        m_BackInput = GetUApi().GetInputByID(UAUIBack).GetPersistentWrapper();

        layoutRoot = GetGame().GetWorkspace().CreateWidgets("OpenZone_PDA/gui/layouts/oz_pda_menu.layout");
        if (!layoutRoot)
            return null;

        m_TabRail   = layoutRoot.FindAnyWidget("TabRail");
        m_StatusLeft  = TextWidget.Cast(layoutRoot.FindAnyWidget("StatusLeft"));
        m_StatusMid   = TextWidget.Cast(layoutRoot.FindAnyWidget("StatusMid"));
        m_StatusRight = TextWidget.Cast(layoutRoot.FindAnyWidget("StatusRight"));
        m_PageHost  = layoutRoot.FindAnyWidget("PageHost");
        m_LockPanel = layoutRoot.FindAnyWidget("LockPanel");
        m_InitPanel = layoutRoot.FindAnyWidget("InitPanel");

        TextWidget itx = TextWidget.Cast(layoutRoot.FindAnyWidget("InitTitle"));
        if (itx)
            itx.SetText("#STR_OZ_INIT_TITLE");
        TextWidget ihx = TextWidget.Cast(layoutRoot.FindAnyWidget("InitHint"));
        if (ihx)
            ihx.SetText("#STR_OZ_INIT_HINT");
        TextWidget ibx = TextWidget.Cast(layoutRoot.FindAnyWidget("BtnInitBigText"));
        if (ibx)
            ibx.SetText("#STR_OZ_DEV_INIT");
        m_BtnClose  = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BtnClose"));

        TextWidget ftx = TextWidget.Cast(layoutRoot.FindAnyWidget("BtnFactoryText"));
        if (ftx)
            ftx.SetText("#STR_OZ_FACTORY_RESET");

        return layoutRoot;
    }

    // Пастка, на якій горіли інші моди: якщо layout не завантажився, синглтон
    // меню лишається «відкритим» і блокує ВСІ меню гри назавжди. Тому перша ж
    // дія при показі -- перевірити, що дерево взагалі є.
    override void OnShow()
    {
        super.OnShow();

        if (!GetLayoutRoot())
        {
            OZ_Log.Error("pda layout failed to load - closing to avoid a ghost menu");
            GetGame().GetUIManager().CloseMenu(OZ_PdaConst.MENU_PDA);
            return;
        }

        SetFocus(layoutRoot);

        // МІСІЮ Й HUD БЕРЕМО ЧЕРЕЗ ЗМІННУ Й ПЕРЕВІРЯЄМО ОБИДВА -- той самий
        // вартовий, що в OZ_LinkMenu ядра. Ланцюжок GetMission().GetHud() без
        // перевірок ядро вже ловило на ACCESS_VIOLATION: на смерті рушій
        // розбирає місію разом із HUD раніше, ніж закриває наші меню.
        Mission mission = GetGame().GetMission();
        if (mission)
        {
            array<string> excludes = new array<string>();
            excludes.Insert("menu");
            mission.AddActiveInputExcludes(excludes);

            // ЗАПАМ'ЯТОВУЄМО, ЯК БУЛО, і повертаємо саме це.
            //
            // OnHide безумовно вмикав ванільний інтерфейс назад -- тобто
            // гравець, який сам його сховав (клавіша HUD), після кожного
            // закриття КПК отримував його назад і мусив ховати знову.
            //
            // Пам'ятаємо ВИДИМІСТЬ ПАНЕЛІ, а не прапорці контексту: Hud.Show()
            // гасить і вмикає кореневу панель, а клавіша HUD і налаштування
            // ставлять лише прапорці, які ховають її частини (ingamehud.c:370
            // проти :900-927). Коли запам'ятовували прапорці, гравець із
            // прихованим клавішею HUD отримував на закритті Show(false) -- і
            // разом із панеллю зникали приціл, підказки дій, швидкий слот і
            // витривалість, до перепідключення.
            //
            // Повторний OnShow без OnHide між ними не перезаписує пам'ять:
            // інакше він запам'ятав би вже НАМИ сховану панель.
            Hud hud = mission.GetHud();
            if (hud)
            {
                if (!m_HudHid)
                    m_HudWasShown = HudPanelShown(hud);
                hud.Show(false);
                m_HudHid = true;
            }
        }

        OZ_ClientState.BindListener(new OZ_PdaMenuListener(this));

        // Рушій може віддати ТОЙ САМИЙ примірник меню на наступному
        // відкритті -- FindMenu перевіряється саме тому. Отже ВЕСЬ стан, що
        // описує пристрій і незавершену дію, а не вікно, треба скидати тут: у
        // руках цілком може бути вже інший КПК.
        //
        // Скидався сам лише m_Sealed. Половина набору піна лишалась від
        // минулого відкриття: клавіатура поверталась у тому ж режимі, з тим
        // же недобраним буфером, і перша ж цифра доводила його до кінця --
        // на ІНШОМУ приладі.
        m_Sealed = false;
        m_PinMode   = "";
        m_PinStep   = 0;
        m_PinBuffer = "";
        m_PinOld    = "";
        m_PinNew    = "";

        // Відлік злому й відлік блокування -- теж стан приладу, а не вікна:
        // минуле відкриття могло скінчитись посеред будь-якого з них.
        m_CrackLive = false;
        DisarmLockout();
        m_HintLockout = false;

        // ПРИВ'ЯЗКА ДО ПРИЛАДУ -- ТУТ І ЛИШЕ ТУТ.
        //
        // Відкрити екран можна тільки дією над предметом у руках
        // (OZ_ActionOpenPda; клавіші відкриття немає з 2026-09-08), тож те, що
        // зараз у руках, і є прилад цього показу. Далі RefreshTick стежить, чи
        // він там само.
        PlayerBase who = PlayerBase.Cast(GetGame().GetPlayer());
        if (who)
            m_Device = OZ_PDA_Base.Cast(who.GetItemInHands());
        else
            m_Device = null;

        // Питаємо сервер, що в нас за пристрій. До відповіді стрічка порожня.
        OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "status", "{}");

        // Заряд і годинник -- ОДРАЗУ, не через секунду: таймер робить перший
        // виток лише за період, і смуга стану відкривалась порожньою.
        LocalStatusTick();

        if (!m_Refresh)
            m_Refresh = new Timer(CALL_CATEGORY_GUI);
        m_Refresh.Run(1.0, this, "RefreshTick", NULL, true);
    }

    override void OnHide()
    {
        super.OnHide();

        if (m_Refresh)
            m_Refresh.Stop();

        // Відкладене закриття могло не встигнути: вікно закрилось раніше
        // іншим шляхом. Парність CallLater/Remove -- правило, а не обережність.
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.Close);

        // ДЕМОНТАЖ СТОРІНОК -- ТУТ, і це не прибирання заради прибирання.
        // Сторінка контактів відписується від статичного інвокера у своєму
        // Unlink() -- але той Unlink мусить хтось покликати. Ніхто не кликав:
        // кожен цикл відкрити/закрити лишав живу сторінку, підписану на
        // відповіді, з мертвими віджетами в руках. Знайшов аудит, а не краш
        // -- крашем воно стало б у першого, хто відкриє КПК двічі й отримає
        // відповідь на роль.
        DropPages();

        OZ_ClientState.BindListener(null);

        // Прив'язка живе рівно один показ (див. m_Device): наступне відкриття
        // застає її порожньою й ставить свою.
        m_Device = null;

        // Той самий вартовий, що в OnShow. Сюди приходять і зі смерті
        // (UIScriptedMenu.OnPlayerDeath -> Close), і коли цей ланцюжок не
        // спрацював -- тоді OnHide може настати вже тоді, коли місії чи HUD
        // немає, і звернення до них валило клієнта.
        bool hudHid = m_HudHid;
        m_HudHid = false;

        Mission mission = GetGame().GetMission();
        if (!mission)
            return;

        array<string> excludes = new array<string>();
        excludes.Insert("menu");
        mission.RemoveActiveInputExcludes(excludes, true);

        // Повертаємо ЛИШЕ те, що самі сховали, і рівно таким, яким воно було.
        Hud hud = mission.GetHud();
        if (hud && hudHid)
            hud.Show(m_HudWasShown);
    }

    // Закритись НАСТУПНИМ КАДРОМ. Для тих, кого кличуть зсередини чужого
    // обходу -- обробника відповіді, ітерації по сторінках, -- і кому не
    // можна руйнувати себе під ногами в того, хто кличе.
    private void CloseLater()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.Close, 0, false);
    }

    // Чи видима ЗАРАЗ коренева панель ванільного HUD -- рівно той віджет,
    // який гасить і вмикає Hud.Show() (IngameHud.Show -> m_HudPanelWidget,
    // ingamehud.c:370). Дістаємо її публічним IngameHud.GetHudPanelWidget()
    // -- так до неї ходить сама місія (missiongameplay.c:1143).
    //
    // Прапорці контексту (HUD_HIDE, HUD_DISABLE) тут питати не можна: вони
    // ховають ЧАСТИНИ панелі (IngameHudVisibility), а не її саму, і
    // повертати панелі їхнє значення означало гасити її цілком.
    //
    // Чужий нащадок Hud без цієї панелі -- «була видима», як і раніше.
    private bool HudPanelShown(Hud hud)
    {
        IngameHud ingame = IngameHud.Cast(hud);
        if (!ingame)
            return true;

        Widget panel = ingame.GetHudPanelWidget();
        if (!panel)
            return true;

        return panel.IsVisible();
    }

    // Кеш для тикових рішень: коли востаннє питали і що бачили netsync.
    private int  m_LastStatusAskMs = 0;
    private int  m_LastSealedAskMs = 0;
    private bool m_SawOn = false;
    private bool m_SawLocked = false;
    private bool m_CrackLive = false;

    void RefreshTick()
    {
        // ПРИЛАД ПІШОВ -- ЕКРАН ІДЕ ЗА НИМ.
        //
        // Меню жило далі: КПК можна було кинути на землю, віддати, втратити
        // разом із тілом -- а відкрите вікно лишалось на екрані й далі
        // питало сервер. Сервер, звісно, відмовляв (ворота дивляться на те,
        // що в руках і в слоті), тож на екрані лишалась остання картинка
        // чужого вже приладу, і гравець читав з неї те, чого більше не має.
        //
        // Питали резолвер («що там у гравця взагалі»), а мусили питати САМ
        // ПРЕДМЕТ. Рішення власника 2026-09-08: екран прив'язаний до приладу,
        // на якому його відкрили, і закривається, щойно в руках не той самий
        // предмет -- поклали в рюкзак, віддали, кинули, померли, взяли інший.
        // Резолвер відповів би тут НАДІТИМ приладом, і вікно поїхало б далі,
        // розмовляючи вже не з тим КПК.
        //
        // Винятку «КПК без предмета» (D132) тут більше немає: те саме рішення
        // прибрало вигаданий термінал зовсім.
        PlayerBase who = PlayerBase.Cast(GetGame().GetPlayer());
        OZ_PDA_Base held;
        if (who)
            held = OZ_PDA_Base.Cast(who.GetItemInHands());

        if (!held || held != m_Device)
        {
            OZ_Log.Dbg("pda: the device left the hands, closing the screen");
            CloseLater();
            return;
        }

        // МЕРТВИЙ, НЕПРИТОМНИЙ ЧИ ЗВ'ЯЗАНИЙ -- ЕКРАН ТЕЖ ІДЕ.
        //
        // Прилад у руках іще не означає рук, які ним користуються: труп
        // тримає, що тримав, а непритомний у машині нічого не впускає.
        // Сервер таким відправникам уже відмовляє (OZ_PdaAccess.Check), тож
        // вікно тільки збирало б відмови. Смерть ваніль закриває й сама
        // (UIScriptedMenu.OnPlayerDeath), і тут -- запасний шлях, як
        // OZ_LinkGate.Tick у ядрі.
        if (!OZ_PdaMenuOpener.CanUse(who))
        {
            OZ_Log.Dbg("pda: the holder is dead, unconscious or restrained, closing the screen");
            CloseLater();
            return;
        }

        // Годинник і заряд -- ЛОКАЛЬНІ: час світу і netsync-поле предмета.
        // Це й прибирає трафік, і чинить заморозку статус-бара на вкладках,
        // які статус не питають.
        LocalStatusTick();

        // Відлік блокування коду -- свій, від дедлайну (див. m_LockedOut).
        LockoutTick();

        // Поки стрічки немає, оновлювати нема кому: замкнений пристрій не
        // віддав жодної сторінки. Питаємо стан самі -- інакше відімкнення
        // іншим шляхом (скинули пін, минув час) лишилось би непоміченим, і
        // екран коду висів би над уже відкритим пристроєм.
        if (!m_Built)
        {
            // ЗАПЕЧАТАНИЙ пристрій status не віддає взагалі: гейт пропускає
            // тільки операції замка (OZ_PdaAccess.IsLockOp), тож цей запит
            // повертається відмовою ГАРАНТОВАНО. Питати його щосекунди --
            // це просити те, у чому зобов'язані відмовити: один сеанс дав
            // 1423 такі відмови, по одній на секунду на гравця, і кожна
            // коштувала повного проходу через ворота на сервері.
            //
            // Перший тік питає все одно, і мусить: доти невідомо, пристрій
            // запечатаний чи просто замкнений, а звичайному замкненому саме
            // status і будує стрічку, щойно код приймуть. Щойно відповідь
            // `sealed` скаже «так», питання припиняються -- і поновлюються
            // самі, коли злам добіжить і та сама відповідь скаже «ні».
            // Замок і живлення -- netsync на предметі: флір видно локально
            // й миттєво, а страховочний запит лишається раз на 5 секунд
            // (пристрій могли відімкнути шляхом, якого netsync не покриє).
            bool askNow = false;
            // Прилад цього показу, а не «якийсь мій»: за нього ми щойно
            // поручились нагорі цього ж тіку.
            OZ_PDA_Base dev = m_Device;
            if (dev)
            {
                // Замок -- за СИНХРОННИМ бітом (OZ_LockedForViewer). Тут
                // стояв OZ_IsUnlocked(), а він читає рядок коду, якого
                // клієнтові не везуть: на клієнті там завжди порожньо, тобто
                // «відімкнено», і зміна замка цей перепит не будила ніколи --
                // лишався тільки п'ятисекундний страховочний.
                bool on     = dev.OZ_IsOn();
                bool locked = dev.OZ_LockedForViewer();
                if (on != m_SawOn || locked != m_SawLocked)
                    askNow = true;
                m_SawOn = on;
                m_SawLocked = locked;
            }

            int nowMs = GetGame().GetTime();
            if (!m_Sealed && (askNow || nowMs - m_LastStatusAskMs >= 5000))
            {
                m_LastStatusAskMs = nowMs;
                OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "status", "{}");
            }

            // Поки йде злам, відлік мусить рухатись щосекунди -- гравець
            // дивиться на нього. Без зламу екран коду статичний, і сталого
            // опиту не заслуговує.
            if (m_PinMode == "unlock")
            {
                if (m_CrackLive || nowMs - m_LastSealedAskMs >= 5000)
                {
                    m_LastSealedAskMs = nowMs;
                    AskSealed();
                }
            }
            return;
        }

        if (m_Current != "" && m_Pages.Contains(m_Current))
            m_Pages.Get(m_Current).OnRefresh();

        OZ_PdaPage mate = Companion();
        if (mate)
            mate.OnRefresh();
    }

    // ----------------------------------------------------------- відповіді

    void HandleResponse(string pageId, string op, bool ok, string json, string error)
    {
        // ОДИН РОЗБІР НА ОДНУ ВІДПОВІДЬ. Той самий status розбирався чотири
        // рази -- стрічка, її перезбирання, замок і смуга стану, -- і кожен
        // читач сам ганяв JsonFileLoader по тому самому рядку. Тепер його
        // розбирають тут, раз, і роздають об'єкт.
        if (pageId == OZ_PdaConst.PAGE_DEVICE && op == "status")
        {
            OZ_PdaDeviceStatus st = null;
            if (ok)
                st = ReadStatus(json);

            // Стрічку будує ПЕРША ж відповідь про пристрій -- і тільки один
            // раз; далі її лише звіряють.
            if (st)
            {
                if (!m_Built)
                    BuildFrom(st);
                else
                    RebuildIfPagesChanged(st);
            }

            ApplyLockState(ok, st, error);
            PaintStatusBar(ok, st);
        }

        if (m_Pages.Contains(pageId))
            m_Pages.Get(pageId).OnResponse(op, ok, json, error);

        if (pageId == OZ_PdaConst.PAGE_DEVICE && op == "unlock")
        {
            if (ok)
            {
                // Код прийнято. Стрічки ще немає -- її будує ПЕРША вдала
                // відповідь про стан, і попросити її мусимо саме тут: доти
                // сторінок немає, а отже нема кому питати.
                EndPin();
                OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "status", "{}");
            }
            else
            {
                OnBadPin(error, json);
            }
        }

        if (pageId == OZ_PdaConst.PAGE_DEVICE && op == "sealed" && ok)
        {
            PaintSealed(json);
            return;
        }

        if (pageId == OZ_PdaConst.PAGE_DEVICE && op == "crack")
        {
            if (!ok)
                PaintPinPrompt("#" + error);
            AskSealed();
            return;
        }

        if (pageId == OZ_PdaConst.PAGE_DEVICE && op == "initiate")
        {
            if (ok)
            {
                // Стрічка вкладок разова, а в новій сесії їх більше:
                // закриваємось, наступне відкриття збере повний набір.
                //
                // НАСТУПНИМ КАДРОМ, а не тут. Close() у UIScriptedMenu --
                // proto native: він руйнує меню негайно, а нас саме зараз
                // кличе OZ_ClientState зсередини свого розбору відповіді й
                // після повернення звертається до себе далі. Знищувати
                // об'єкт із його ж зворотного виклику -- звернення по
                // мертвому вказівнику, і те, що воно досі не впало, нічого
                // не обіцяє.
                CloseLater();
            }
            return;
        }

        if (pageId == OZ_PdaConst.PAGE_DEVICE && op == "factory_reset")
        {
            if (ok)
            {
                // Пристрій щойно став чистим і відімкненим: екран коду геть,
                // стан перепитуємо -- стрічка збудується з нього.
                EndPin();
                OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "status", "{}");
            }
            else
            {
                PaintPinPrompt("#" + error);
            }
            return;
        }

        if (pageId == OZ_PdaConst.PAGE_DEVICE && op == "setpin")
        {
            if (ok)
            {
                // Код прийнято -- екран іде геть, а стан перепитує сама
                // сторінка: писати «пін задано» з голови клієнта означало б
                // показати те, чого сервер міг і не зробити.
                EndPin();
                OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "status", "{}");
            }
            else
            {
                OnBadPin(error, json);
            }
        }
    }

    // Розбір відповіді на status -- єдиний у меню.
    //
    // КОПІЯ, а не розібраний об'єкт: BuildFrom створює віджет на кожну
    // вкладку, а Pages -- вкладений масив, який виділив серіалізатор, --
    // читається і до цього, і після (шапка OZ_PdaTypes).
    private OZ_PdaDeviceStatus ReadStatus(string json)
    {
        string err;
        OZ_PdaDeviceStatus parsed = new OZ_PdaDeviceStatus();
        if (!JsonFileLoader<OZ_PdaDeviceStatus>.LoadData(json, parsed, err))
        {
            OZ_Log.Error("device status unreadable: " + err);
            return null;
        }
        return parsed.Copy();
    }

    // НАБІР СТОРІНОК МІНЯЄТЬСЯ ЗА ЖИТТЯ ВІКНА, і стрічка мусить за ним іти.
    //
    // Сторінку вмикає не лише профіль, а й ВСТАВЛЕНИЙ МОДУЛЬ: дістав рацію з
    // відсіку -- вкладка рації мусить зникнути, вставив -- з'явитись. Стрічка
    // ж будувалась рівно один раз, першою відповіддю, і після цього не
    // мінялась ніколи: гравець міняв залізо, дивлячись на вкладки, яких у
    // приладі вже немає. Натискання на таку вкладку йшло на сервер і чесно
    // отримувало відмову, якої ніхто не пояснював.
    //
    // Прилад сам штовхає стан на кожне під'єднання й від'єднання
    // (OZ_PDA_Base.EEItemAttached -> PushState), тож окремого опиту не треба.
    private void RebuildIfPagesChanged(OZ_PdaDeviceStatus st)
    {
        if (!st || !st.Pages)
            return;

        // Порядок теж значущий: його задає профіль, і зміна порядку -- це
        // інша стрічка.
        bool same = st.Pages.Count() == m_PageOrder.Count();
        if (same)
        {
            for (int i = 0; i < st.Pages.Count(); i++)
            {
                if (st.Pages[i] != m_PageOrder[i])
                {
                    same = false;
                    break;
                }
            }
        }

        if (same)
            return;

        // Що було відкрито -- лишаємо відкритим, якщо воно ще є.
        string keep = m_Current;

        DropPages();
        BuildFrom(st);

        if (keep != "" && m_Pages.Contains(keep))
            Select(keep);
    }

    // Знести сторінки й стрічку, лишивши вікно живим. Спільне для
    // перезбирання й для закриття -- інакше друге місце неминуче забуде
    // покликати Unlink, і сторінка лишиться підписаною з мертвими віджетами.
    private void DropPages()
    {
        if (m_Pages)
        {
            for (int i = 0; i < m_Pages.Count(); i++)
            {
                OZ_PdaPage page = m_Pages.GetElement(i);
                if (page)
                    page.Unlink();
            }
            m_Pages.Clear();
        }

        if (m_Tabs)
        {
            for (int t = 0; t < m_Tabs.Count(); t++)
            {
                if (m_Tabs[t])
                    m_Tabs[t].Unlink();
            }
            m_Tabs.Clear();
        }

        m_PageOrder.Clear();
        m_Current = "";
        m_Built = false;
    }

    // Стрічка, як її збудували: за нею й звіряємось.
    private ref array<string> m_PageOrder = new array<string>();

    // st -- уже КОПІЯ (ReadStatus): нижче AddTab створює віджет на кожну
    // вкладку, а Pages читається і перед цим, і після.
    private void BuildFrom(OZ_PdaDeviceStatus st)
    {
        if (!st || !st.Pages)
            return;

        // ЗАЩІПКА СТАВИТЬСЯ В КІНЦІ, і лише коли стрічка справді з'явилась.
        //
        // Тут вона стояла на початку й безумовно. Профіль без сторінок (чи
        // такий, жодну сторінку якого цей клієнт малювати не вміє) давав
        // порожню стрічку -- і разом із нею глушив опит стану в RefreshTick,
        // бо той працює рівно доти, доки m_Built == false. Вікно лишалось
        // порожнім назавжди й перепитати вже не могло.

        // ФРАКЦІЯ ЖИВЕ В ОДНІЙ ВКЛАДЦІ З КОНТАКТАМИ (рішення власника
        // 2026-08-30): ліворуч люди, праворуч свої. Це те саме питання --
        // «хто навколо і чиї вони», -- і розводити його на дві вкладки
        // означало клацати між ними на кожну думку.
        //
        // Сторінки лишаються ДВІ: два серверні обробники, два конверти, два
        // незалежні оновлення. Спільна в них тільки вкладка -- і рівно це
        // тут і зроблено, без злиття коду сторінок в одну купу.
        //
        // Пару оголошує ТОЙ, ХТО ЇЇ УТВОРЮЄ (OZ_PdaPageFactory.Pair), а не
        // це меню. Тут стояло ім'я фракційної сторінки -- тобто КПК знав про
        // мод, якого може й не бути; після виносу фракцій окремим модом таке
        // знання стало прямою залежністю на порожнє місце.
        //
        // Якщо профіль дав одну сторінку пари без другої, вона отримує власну
        // вкладку, як і раніше: приліпити її нема до чого.
        m_Companion = "";
        for (int c = 0; c < st.Pages.Count(); c++)
        {
            string mate = OZ_PdaPageFactory.CompanionOf(st.Pages[c]);
            if (mate != "" && st.Pages.Find(mate) != -1)
            {
                m_Companion = mate;
                break;
            }
        }

        // Порядок задає ПРОФІЛЬ, не реєстр: адмін вирішує, що йде першим.
        m_PageOrder.Clear();
        for (int i = 0; i < st.Pages.Count(); i++)
        {
            AddTab(st.Pages[i]);
            m_PageOrder.Insert(st.Pages[i]);
        }

        // Рельс -- WrapSpacerWidgetClass, що міряє себе сам: щойно останню
        // вкладку вставлено, він мусить перерахувати свою висоту.
        if (m_TabRail)
            m_TabRail.Update();

        // Жодної сторінки не вийшло -- нічого й не защіпаємо: хай опит іде
        // далі, а в лозі лишається причина.
        if (m_Pages.Count() == 0)
        {
            OZ_Log.Warn("pda: the device status brought no page this client can draw - the tab rail stays empty");
            return;
        }

        m_Built = true;

        if (st.Pages.Count() > 0)
            Select(st.Pages[0]);
    }

    // Сторінка, що ділить вкладку з контактами. Порожньо -- ділити нема чому.
    private string m_Companion = "";

    // Чия це вкладка. Для сторінки-супутника -- вкладка контактів.
    private string TabOf(string pageId)
    {
        if (m_Companion != "" && pageId == m_Companion)
            return OZ_PdaConst.PAGE_CONTACTS;
        return pageId;
    }

    private void AddTab(string pageId)
    {
        OZ_PdaPage page = OZ_PdaPageFactory.Make(pageId);
        if (!page)
            return;   // клієнт не вміє малювати -- вкладки не буде, причина в лозі

        // Супутник отримує сторінку, але НЕ вкладку: його показує та сама
        // кнопка, що й контакти.
        if (pageId != m_Companion)
        {
            Widget tab = GetGame().GetWorkspace().CreateWidgets("OpenZone_PDA/gui/layouts/oz_pda_tab.layout", m_TabRail);
            if (!tab)
                return;

            tab.SetName(pageId);
            tab.SetUserID(1);         // так OnClick відрізняє вкладку від решти

            // Набір ЗВІДКИ БРАТИ картинку більше не наш: Icon() віддає повне
            // посилання "set:<набір> image:<спрайт>", і склейка рації чи
            // фракцій називає в ньому СВІЙ набір (рішення власника
            // 2026-09-09). Тут стояло "set:oz_pda_icons image:" + ..., тобто
            // рейка вміла малювати лише з атласа КПК -- і чужі значки мусили
            // лежати в ньому.
            ImageWidget icon = ImageWidget.Cast(tab.FindAnyWidget("TabIcon"));
            if (icon)
                icon.LoadImageFile(0, OZ_PdaPageFactory.Icon(pageId));

            TextWidget label = TextWidget.Cast(tab.FindAnyWidget("TabLabel"));
            if (label)
                label.SetText(OZ_PdaPageFactory.Title(pageId));

            m_Tabs.Insert(tab);
        }

        page.Init(pageId, m_PageHost);
        page.Show(false);
        m_Pages.Insert(pageId, page);
    }

    void Select(string pageId)
    {
        // Обрати супутник -- це обрати вкладку, у якій він живе. Інакше
        // «перейди на фракцію» лишило б половину екрана порожньою: вкладки
        // з таким іменем немає, а пара, яку вона показує, сховалась би.
        if (m_Companion != "" && pageId == m_Companion)
            pageId = TabOf(pageId);

        if (m_Current == pageId)
            return;

        // Сторінки, якої немає, не існує й вибір: інакше поточна ховається,
        // нова не показується, і КПК стоїть порожнім екраном.
        if (!m_Pages.Contains(pageId))
            return;

        if (m_Current != "" && m_Pages.Contains(m_Current))
        {
            m_Pages.Get(m_Current).Show(false);
            m_Pages.Get(m_Current).OnDeselected();
        }

        // Супутник ховається разом зі своєю парою -- і показується разом.
        if (m_Companion != "" && m_Pages.Contains(m_Companion))
        {
            if (TabOf(m_Companion) == TabOf(m_Current))
            {
                m_Pages.Get(m_Companion).Show(false);
                m_Pages.Get(m_Companion).OnDeselected();
            }
        }

        m_Current = pageId;

        if (m_Pages.Contains(pageId))
        {
            m_Pages.Get(pageId).Show(true);
            m_Pages.Get(pageId).OnSelected();
        }

        if (m_Companion != "" && m_Pages.Contains(m_Companion))
        {
            if (TabOf(m_Companion) == TabOf(pageId) && m_Companion != pageId)
            {
                m_Pages.Get(m_Companion).Show(true);
                m_Pages.Get(m_Companion).OnSelected();
            }
        }

        PaintTabs();
    }

    // Сторінка на ім'я, або null. Потрібна сусідці по вкладці: фракція
    // питає контакти, кого там вибрано, і саме тому вони разом.
    OZ_PdaPage PageOf(string pageId)
    {
        if (!m_Pages || !m_Pages.Contains(pageId))
            return null;
        return m_Pages.Get(pageId);
    }

    // Сторінка-супутник поточної, або порожньо. Одна відповідь на питання
    // «кого ще стосується те, що зараз на екрані»: оновлення, кліки, миша.
    private OZ_PdaPage Companion()
    {
        if (m_Companion == "" || m_Current == "")
            return null;
        if (m_Companion == m_Current)
            return null;
        if (TabOf(m_Companion) != TabOf(m_Current))
            return null;
        if (!m_Pages.Contains(m_Companion))
            return null;
        return m_Pages.Get(m_Companion);
    }

    private void PaintTabs()
    {
        for (int i = 0; i < m_Tabs.Count(); i++)
        {
            Widget t = m_Tabs[i];
            bool active = (t.GetName() == m_Current);

            Widget pick = t.FindAnyWidget("TabActive");
            if (pick)
                pick.Show(active);

            Widget mark = t.FindAnyWidget("TabMark");
            if (mark)
                mark.Show(active);

            ImageWidget icon = ImageWidget.Cast(t.FindAnyWidget("TabIcon"));
            if (icon)
            {
                if (active)
                    icon.SetColor(OZ_Palette.ACCENT);
                else
                    icon.SetColor(OZ_Palette.MUTED);
            }

            TextWidget label = TextWidget.Cast(t.FindAnyWidget("TabLabel"));
            if (label)
            {
                if (active)
                    label.SetColor(OZ_Palette.ACCENT);
                else
                    label.SetColor(OZ_Palette.MUTED);
            }
        }
    }

    // ---------------------------------------------------------------- замок

    private void ApplyLockState(bool ok, OZ_PdaDeviceStatus st, string error)
    {
        if (!m_LockPanel)
            return;

        // Відмова саме через замок -- єдина причина показати екран коду.
        //
        // І це ЗВИЧАЙНИЙ шлях, а не крайній випадок: замкнений пристрій не
        // віддає навіть device/status, тож нормально ми потрапляємо сюди, а
        // не в розбір відповіді нижче. Показати саму панель мало -- треба
        // ввімкнути режим, інакше цифри нікуди не йдуть: панель видно, а
        // кнопки мовчать. Саме так це й виглядало на живому клієнті.
        if (!ok)
        {
            // Замок -- ЄДИНА причина показати екран коду. Сервер відмовляє
            // status'у на замкненому пристрої саме кодом STR_OZ_ERR_LOCKED
            // (OZ_PdaAccess: status не входить у IsLockOp). Раніше клієнт
            // чекав тут NO_ACCESS -- код, який на замок НЕ приходить, тож
            // екран коду не з'являвся ЖОДНОГО разу, а прилад без піна над
            // яким висів NO_ACCESS (немає девайса зовсім) навпаки діставав
            // незакриваний пад. Тепер матчимо саме LOCKED.
            if (error == "STR_OZ_ERR_LOCKED")
            {
                if (m_PinMode != "unlock")
                    BeginPin("unlock");

                // Запечатаний пристрій показує СВОЄ. Пропонувати набирати код
                // там, де його ніхто не знає, -- це запрошення в нікуди.
                AskSealed();
            }
            else if (m_PinMode == "unlock")
            {
                EndPin();
            }
            return;
        }

        if (!st)
            return;

        m_HasPin = st.HasPin;
        SetPinLengths(st.PinLength, st.NewPinLength);

        // Нічийний пристрій -- ЕКРАН ІНІЦІАЦІЇ замість сторінок: після
        // factory reset (чи зі свіжим приладом) єдина доступна дія --
        // зробити його своїм.
        ShowInit(!st.Owned && st.Powered && st.Unlocked);

        // Вимкнений прилад не має екрана коду -- і НАБОРУ теж: буфер і
        // крок пережили б вимикання, і наступні цифри доклеювались би до
        // мертвого стану (зміряно живим тестом 2026-08-29: «зміна коду»
        // з двох сеансів набору). Вимкнули -- набір скінчився.
        if (!st.Powered)
        {
            if (m_PinMode != "")
                EndPin();
            return;
        }

        bool needPin = st.HasPin && !st.Unlocked;

        if (needPin)
        {
            // Примусовий екран б'є будь-який добровільний: замкнений пристрій
            // не місце для зміни коду.
            if (m_PinMode != "unlock")
                BeginPin("unlock");

            // Відлік -- той самий, що з відмов і з `sealed`: один дедлайн і
            // одне місце, яке його малює.
            if (st.LockedOut)
                ArmLockout(st.LockWaitS);
            else
                DisarmLockout();
            PaintPinPrompt("");
            return;
        }

        // Відімкнули -- примусовий екран іде геть. Добровільний лишається:
        // гравець сам його відкрив і сам закриє.
        if (m_PinMode == "unlock")
            EndPin();
    }

    // ------------------------------------------------------------ екран коду

    // Просить сторінка «Пристрій» -- через FindMenu, а не через посилання:
    // меню одне, живе в UIManager, і тримати на нього другу нитку означало б
    // мати два джерела правди про те, чи воно взагалі відкрите.
    private void ShowInit(bool show)
    {
        m_InitShown = show;
        SyncPanels();
    }

    // ОДНА ТОЧКА, ЯКА ВИРІШУЄ ПРО ВСІ ЧОТИРИ ВІДЖЕТИ.
    //
    // Видимість панелі ініціалізації, екрана коду, поля сторінок і стрічки
    // вкладок пов'язана: рівно одна з перших двох може бути на екрані, і поки
    // хоч одна з них є -- сторінок бути не може. Це правило жило в трьох
    // місцях по шматку (ShowInit, BeginPin, EndPin), і кожне знало лише
    // частину: EndPin, наприклад, безумовно вмикав поле сторінок -- зокрема й
    // поверх відкритої панелі ініціалізації.
    //
    // Стан описують ДВІ змінні, а малює їх ця функція. Хто міняє стан --
    // кличе її й більше нічого не показує сам.
    private bool m_InitShown = false;

    private void SyncPanels()
    {
        bool pin = m_PinMode != "";

        if (m_InitPanel)
            m_InitPanel.Show(m_InitShown);

        if (m_LockPanel)
            m_LockPanel.Show(pin);

        // Сторінки ховаємо ОКРЕМО, а не покладаємось на те, що панель їх
        // перекриє: 3D-прев'ю предмета малюється власним проходом рушія і
        // проступає крізь будь-який 2D-віджет поверх нього. Виміряно на
        // живому клієнті -- модель було видно просто крізь екран коду.
        if (m_PageHost)
            m_PageHost.Show(!m_InitShown && !pin);

        if (m_TabRail)
            m_TabRail.Show(!m_InitShown);
    }

    void BeginPin(string mode)
    {
        if (!m_LockPanel)
            return;

        m_PinMode   = mode;
        m_PinBuffer = "";
        m_PinOld    = "";
        m_PinNew    = "";

        // Задати код там, де його ще немає, -- це одразу новий код, без
        // питання про старий.
        if (mode == "set" && !m_HasPin)
            m_PinStep = 1;
        else
            m_PinStep = 0;

        SyncPanels();

        PaintPinPrompt("");
        PaintPinDots();

        // Скидання до заводських -- лише на ПРИМУСОВОМУ екрані коду: там
        // стоїть той, хто коду не знає. Добровільні set/clear -- власник.
        ButtonWidget fac = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BtnFactory"));
        if (fac)
            fac.Show(mode == "unlock");
    }

    // Запечатаний пристрій не віддає навіть status, тож про його стан
    // доводиться питати ОКРЕМО -- операцією, яку гейт пропускає.
    private void AskSealed()
    {
        OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "sealed", "{}");
    }

    // Запечатаний пристрій має СВОЮ розповідь, і клавіатура в ній не бере
    // участі: набирати код, якого ніхто не знає, нема сенсу.
    private void PaintSealed(string json)
    {
        string err;
        OZ_PdaDeviceStatus st = new OZ_PdaDeviceStatus();
        if (!JsonFileLoader<OZ_PdaDeviceStatus>.LoadData(json, st, err))
            return;

        // Єдине місце, де цей прапорець ставиться. Коли злам добігає, сервер
        // прибирає код, пристрій перестає бути запечатаним, і та сама
        // відповідь сама ж поверне сюди false -- після чого RefreshTick знову
        // спитає status і збудує стрічку. Окремого сигналу «зламано» не
        // треба саме тому.
        m_Sealed = st.Sealed;
        SetPinLengths(st.PinLength, st.NewPinLength);

        // Поки йде злам, RefreshTick питає sealed щосекунди (живий відлік);
        // без зламу -- раз на 5 секунд, екран коду статичний.
        m_CrackLive = st.Cracking;

        // Ця відповідь -- єдина, яку замкнений прилад отримує сам, тож і про
        // блокування вона каже першою. Кожна наступна звіряє дедлайн із
        // сервером. Малюється відлік нижче, коли ясно, що підказка не
        // зайнята зламом чи печаткою.
        if (st.LockedOut)
            ArmLockout(st.LockWaitS);
        else
            DisarmLockout();

        ButtonWidget crack = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BtnCrack"));
        Widget pad = layoutRoot.FindAnyWidget("LockPad");
        TextWidget hint = TextWidget.Cast(layoutRoot.FindAnyWidget("LockHint"));

        // ПАД ХОВАЄ ЛИШЕ ПЕЧАТКА: у запечатаного коду не набирають, його
        // ламають. Звичайний замкнений КПК лишається з клавіатурою, а
        // DECRYPT стоїть під нею (рішення власника 2026-08-28).
        if (pad)
            pad.Show(!st.Sealed);

        if (st.Sealed)
        {
            TextWidget label = TextWidget.Cast(layoutRoot.FindAnyWidget("LockLabel"));
            if (label)
                label.SetText("#STR_OZ_SEALED");

            TextWidget dots = TextWidget.Cast(layoutRoot.FindAnyWidget("LockDots"));
            if (dots)
                dots.SetText("");
        }

        // ВІДЛІК ЗЛАМУ -- ОДИН НА ОБИДВА СТАНИ. Гілка «йде злам» стояла
        // двічі, слово в слово: раз для запечатаного, раз для звичайного.
        if (st.Cracking)
        {
            if (crack)
                crack.Show(false);
            if (hint)
            {
                string left = "#STR_OZ_CRACKING";
                left += "   " + st.CrackLeftSec.ToString() + " s";
                hint.SetText(left);
                m_HintLockout = false;
            }
            return;
        }

        if (crack)
        {
            crack.Show(st.HasDecryptor);
            if (st.HasDecryptor)
            {
                TextWidget ct = TextWidget.Cast(layoutRoot.FindAnyWidget("BtnCrackText"));
                if (ct)
                    ct.SetText("#STR_OZ_CRACK");
            }
        }

        // Підказка про потрібну плату -- лише в запечатаного: у звичайного
        // замкненого є пад, і сказати йому «потрібен дешифратор» означало б
        // порадити не той шлях.
        if (hint)
        {
            if (st.Sealed && !st.HasDecryptor)
            {
                hint.SetText("#STR_OZ_SEALED_NEED");
                m_HintLockout = false;
            }
            else if (st.Sealed)
            {
                hint.SetText("");
                m_HintLockout = false;
            }
        }

        // Звичайний замкнений прилад, злому немає: під падом -- відлік
        // блокування, поки воно триває. Щойно сервер сказав «скінчилось» --
        // звичайна підказка, але лише замість НАШОГО відліку: помилку, яку
        // щойно показала відмова, ця відповідь не затирає.
        if (!st.Sealed)
        {
            if (LockoutLive())
                PaintLockHint();
            else if (m_HintLockout)
                PaintPinPrompt("");
        }
    }

    void EndPin()
    {
        m_PinMode   = "";
        m_PinStep   = 0;
        m_PinBuffer = "";
        m_PinOld    = "";
        m_PinNew    = "";

        SyncPanels();

        // Панель вимкнули -- усе, що жило лише на ній, теж. Інакше кнопка
        // зламу лишалась би видимою на вже відкритому пристрої.
        ButtonWidget crack = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BtnCrack"));
        if (crack)
            crack.Show(false);

        ButtonWidget fac2 = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BtnFactory"));
        if (fac2)
            fac2.Show(false);

        Widget pad = layoutRoot.FindAnyWidget("LockPad");
        if (pad)
            pad.Show(true);
    }

    private void PaintPinPrompt(string hintKey)
    {
        TextWidget label = TextWidget.Cast(layoutRoot.FindAnyWidget("LockLabel"));
        if (label)
        {
            if (m_PinMode == "unlock")
                label.SetText("#STR_OZ_LOCK_PROMPT");
            else if (m_PinStep == 0)
                label.SetText("#STR_OZ_PIN_OLD");
            else if (m_PinStep == 1)
                label.SetText("#STR_OZ_PIN_NEW");
            else
                label.SetText("#STR_OZ_PIN_REPEAT");
        }

        // Звичайна підказка під час блокування -- це саме блокування: «цифри,
        // Enter» кликали б набирати код, якого зараз не слухатимуть.
        if (hintKey == "" && LockoutLive())
        {
            PaintLockHint();
            return;
        }

        m_HintLockout = false;
        TextWidget hint = TextWidget.Cast(layoutRoot.FindAnyWidget("LockHint"));
        if (hint)
        {
            if (hintKey != "")
                hint.SetText(hintKey);
            else
                hint.SetText("#STR_OZ_PIN_HINT");
        }
    }

    // ------------------------------------------------------- блокування коду

    // ВІДЛІК БЛОКУВАННЯ -- ЛОКАЛЬНИЙ, ВІД ДЕДЛАЙНУ.
    //
    // Відлік жив лише у вдалій відповіді на status, а замкнений прилад її не
    // отримує ніколи: ворота відмовляють раніше (STR_OZ_ERR_LOCKED). Гравець,
    // що вичерпав спроби, бачив «Хибний код» і набирав далі, не знаючи, що
    // його не слухають. Тепер секунди везуть самі відмови unlock/setpin
    // (STR_OZ_LOCK_TOO_MANY з тілом) і відповідь `sealed`, а далі меню рахує
    // саме: дедлайн = зараз + LockWaitS (сервер округлює вгору, див.
    // ArmLockout), і секундний RefreshTick перемальовує підказку. Кожна нова
    // відповідь сервера ставить дедлайн заново.
    //
    // Дедлайн нуль -- рахувати нема чого: блок до рестарту сервера або тіло
    // відмови не прочиталось. Тоді пишемо саму причину, без числа, і чекаємо
    // наступного слова сервера.
    private bool m_LockedOut   = false;
    private int  m_LockUntilMs = 0;

    // Чи підказка пада ЗАРАЗ показує саме відлік. Такт перемальовує лише
    // свій текст: помилку, яку щойно сказав сервер, він не затирає.
    private bool m_HintLockout = false;

    private void ArmLockout(int waitS)
    {
        // Нуль сервер пише лише для блоку до рестарту, але сюди ж падає
        // відмова, чиє тіло не прочиталось. Якщо дедлайн цього ж блокування
        // вже відомий -- це друге: лишаємо живий відлік, а не міняємо його на
        // безстрокове «забагато спроб».
        if (waitS <= 0 && LockoutLive() && m_LockUntilMs != 0)
            return;

        m_LockedOut   = true;
        m_LockUntilMs = 0;

        // Сервер округлює секунди вгору (OZ_PDA_Base.OZ_LockWaitSec), тож
        // справжній залишок не більший за LockWaitS: нуль клієнта не настає
        // раніше за нуль сервера, і код, набраний на нулі, сервер уже слухає.
        // Секунду зверху клієнт більше не додає -- з нею свіжі 300 с
        // показувались як «301 s».
        if (waitS > 0)
            m_LockUntilMs = GetGame().GetTime() + waitS * 1000;
    }

    private void DisarmLockout()
    {
        m_LockedOut   = false;
        m_LockUntilMs = 0;
    }

    // Чи блокування ще триває. Минулий дедлайн знімає його тут-таки: хто б
    // не спитав, бачить ту саму правду, а не чекає на такт.
    private bool LockoutLive()
    {
        if (!m_LockedOut)
            return false;

        if (m_LockUntilMs != 0 && GetGame().GetTime() >= m_LockUntilMs)
        {
            DisarmLockout();
            return false;
        }
        return true;
    }

    private void PaintLockHint()
    {
        TextWidget hint = TextWidget.Cast(layoutRoot.FindAnyWidget("LockHint"));
        if (!hint)
            return;

        string wait = Widget.TranslateString("#STR_OZ_LOCK_TOO_MANY");
        if (m_LockUntilMs != 0)
        {
            // Угору до цілої секунди: «0 s» на екрані, поки код ще не
            // слухають, було б неправдою.
            int leftMs = m_LockUntilMs - GetGame().GetTime();
            int secs = (leftMs + 999) / 1000;
            if (secs > 0)
                wait += "  (" + secs.ToString() + " s)";
        }
        hint.SetText(wait);
        m_HintLockout = true;
    }

    // Секундний такт відліку (RefreshTick). Дійшов до нуля -- повертаємо
    // звичайну підказку.
    private void LockoutTick()
    {
        if (!m_HintLockout)
            return;

        if (LockoutLive())
            PaintLockHint();
        else
            PaintPinPrompt("");
    }

    // Enter натиснуто. Що це означає -- залежить від режиму й кроку.
    private void PinConfirm()
    {
        if (m_PinMode == "unlock")
        {
            OZ_PdaPinAttempt att = new OZ_PdaPinAttempt();
            att.Pin = m_PinBuffer;

            string ajson;
            string aerr;
            if (JsonFileLoader<OZ_PdaPinAttempt>.MakeData(att, ajson, aerr, false))
                OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "unlock", ajson);
            return;
        }

        if (m_PinMode == "clear")
        {
            m_PinOld = m_PinBuffer;
            SendPinChange(m_PinOld, "");
            return;
        }

        // m_PinMode == "set"
        if (m_PinStep == 0)
        {
            m_PinOld    = m_PinBuffer;
            m_PinBuffer = "";
            m_PinStep   = 1;
            PaintPinPrompt("");
            PaintPinDots();
            return;
        }

        if (m_PinStep == 1)
        {
            m_PinNew    = m_PinBuffer;
            m_PinBuffer = "";
            m_PinStep   = 2;
            PaintPinPrompt("");
            PaintPinDots();
            return;
        }

        // Повтор не збігся -- повертаємось на крок назад, а не мовчки
        // приймаємо перший варіант.
        if (m_PinBuffer != m_PinNew)
        {
            m_PinNew    = "";
            m_PinBuffer = "";
            m_PinStep   = 1;
            PaintPinPrompt("#STR_OZ_PIN_MISMATCH");
            PaintPinDots();
            return;
        }

        SendPinChange(m_PinOld, m_PinNew);
    }

    private void SendPinChange(string oldPin, string newPin)
    {
        OZ_PdaPinChange ch = new OZ_PdaPinChange();
        ch.OldPin = oldPin;
        ch.NewPin = newPin;

        string json;
        string err;
        if (JsonFileLoader<OZ_PdaPinChange>.MakeData(ch, json, err, false))
            OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "setpin", json);
    }

    // Смуга стану -- те, що гравець мусить бачити, не заходячи на сторінку:
    // живлення, стан зв'язку й час. Ті самі дані, що вже прийшли; окремого
    // запиту не робимо.
    // Локальна половина статус-бара: заряд із netsync-поля предмета й
    // годинник світу. Сервер тут ні до чого.
    private void LocalStatusTick()
    {
        TextWidget left  = m_StatusLeft;
        TextWidget right = m_StatusRight;

        // Заряд -- ПРИЛАДУ ЦЬОГО ЕКРАНА. Резолвер тут відповів би надітим,
        // щойно прилад покинув руки, і смуга стану на один тік показала б
        // чужий відсоток (рішення власника 2026-09-08).
        OZ_PDA_Base dev = m_Device;

        if (left && dev)
        {
            if (!dev.OZ_IsOn())
            {
                left.SetText("#STR_OZ_DEV_OFF");
            }
            else
            {
                int pct = Math.Round(dev.OZ_Charge01() * 100);
                string l = "#STR_OZ_DEV_POWER";
                l += "  " + pct.ToString() + "%";
                left.SetText(l);
            }
        }

        if (right)
        {
            int y, mo, d, h, m;
            GetGame().GetWorld().GetDate(y, mo, d, h, m);
            string tm = OZ_Time.Pad2(h);
            tm += ":" + OZ_Time.Pad2(m);
            right.SetText(tm);
        }
    }

    // СЕРЕДНЯ КОМІРКА -- І БІЛЬШ НІЩО.
    //
    // Заряд і годинник тут малювались теж, а LocalStatusTick переписує обидва
    // щосекунди з локальної сутності й ігрового годинника -- тобто ця половина
    // роботи жила рівно до наступного такту. Своє в цієї відповіді одне: чи
    // пристрій ще онлайн, бо це знає лише сервер.
    private void PaintStatusBar(bool ok, OZ_PdaDeviceStatus st)
    {
        if (!m_StatusMid)
            return;

        if (!ok)
        {
            m_StatusMid.SetText("");
            return;
        }

        if (!st)
            return;

        if (st.Online)
            m_StatusMid.SetText("");
        else
            m_StatusMid.SetText("#STR_OZ_DEV_OFFLINE_SHORT");
    }


    // true -- натискання було по цифровій панелі й уже розібране.
    private bool PinPadClick(string name)
    {
        if (name == "KeyOk")
        {
            if (m_PinBuffer.Length() > 0)
                PinConfirm();
            return true;
        }

        if (name == "KeyBack")
        {
            if (m_PinBuffer.Length() > 0)
            {
                m_PinBuffer = m_PinBuffer.Substring(0, m_PinBuffer.Length() - 1);
                PaintPinDots();
            }
            return true;
        }

        // Key0..Key9 -- і тільки вони: чуже ім'я тут не цифра.
        if (name.Length() != 4)
            return false;
        if (name.Substring(0, 3) != "Key")
            return false;

        string digit = name.Substring(3, 1);
        if (digit != "0" && digit.ToInt() == 0)
            return false;

        if (m_PinBuffer.Length() < PadLength())
        {
            m_PinBuffer += digit;
            PaintPinDots();
        }
        return true;
    }

    // ДОВЖИНА КОДУ ПРИЇЖДЖАЄ З СЕРВЕРА (ТЗ-5 R-B3.3). Тут стояла четвірка
    // ТРЬОМА літералами -- у наборі мишею, у крапках і на клавіатурі
    // (OnKeyPress), -- і вона була ЄДИНИМ правилом на весь мод: сервер
    // довжини не перевіряв узагалі.
    //
    // ДОВЖИН ДВІ, і котра з них правило пада -- вирішує крок. Поки число
    // було одне (довжина з профілю), адмін, що зменшив PinLength профілю з
    // шести до чотирьох, лишав власників шестизначних кодів із падом, який
    // шостої цифри вже не приймав: відімкнути свій прилад вони могли тільки
    // скиданням. Тепер сервер каже обидва:
    //
    //   PinLength    -- код, що СТОЇТЬ на приладі: його набирають, щоб
    //                   відімкнути, і його ж як СТАРИЙ при зміні чи знятті;
    //   NewPinLength -- скільки цифр профіль вимагає від НОВОГО коду та
    //                   його повтору.
    //
    // Нуль -- сервер не сказав: умовчання, а не пад, що не приймає жодної
    // цифри.
    private int m_PinLenCur = OZ_PdaConst.PIN_LENGTH_DEFAULT;
    private int m_PinLenNew = OZ_PdaConst.PIN_LENGTH_DEFAULT;

    private void SetPinLengths(int cur, int neu)
    {
        int before = PadLength();

        m_PinLenCur = OZ_PdaConst.PIN_LENGTH_DEFAULT;
        if (cur > 0)
            m_PinLenCur = cur;

        m_PinLenNew = OZ_PdaConst.PIN_LENGTH_DEFAULT;
        if (neu > 0)
            m_PinLenNew = neu;

        if (PadLength() == before)
            return;

        // Набране до зміни -- уже не за тим правилом. Чистимо, а не ріжемо:
        // половина коду в буфері гірша за порожній.
        m_PinBuffer = "";
        PaintPinDots();
    }

    // Скільки цифр приймає пад САМЕ ЗАРАЗ. Новий код і його повтор -- лише
    // на кроках 1 і 2 зміни; решта (відімкнути, старий код, зняти код) --
    // код, що стоїть.
    private int PadLength()
    {
        if (m_PinMode == "set" && m_PinStep > 0)
            return m_PinLenNew;
        return m_PinLenCur;
    }

    private void PaintPinDots()
    {
        TextWidget dots = TextWidget.Cast(layoutRoot.FindAnyWidget("LockDots"));
        if (!dots)
            return;

        int len = PadLength();
        string s = "";
        for (int i = 0; i < len; i++)
        {
            if (i < m_PinBuffer.Length())
                s += "*";
            else
                s += "-";
        }
        dots.SetText(s);
    }

    // Причину каже СЕРВЕР, і показувати треба саме її. Раніше тут завжди
    // писалось «Wrong code», і через це «пристрою немає» -- гравець помер, а
    // КПК лишився на трупі -- виглядало як невірний код. Півгодини пішло на
    // те, щоб зрозуміти, що вводити нема куди.
    private void OnBadPin(string error, string json)
    {
        m_PinBuffer = "";

        // На кроці «повтори новий код» помилятись нема в чому -- сервер
        // відмовляє лише через СТАРИЙ код, тож повертаємо на його крок.
        if (m_PinMode == "set" && m_PinStep > 0 && m_HasPin)
        {
            m_PinStep = 0;
            m_PinOld  = "";
            m_PinNew  = "";
        }

        // Крапки -- ПІСЛЯ кроку: довжина пада від нього залежить.
        PaintPinDots();

        // БЛОКУВАННЯ ПРИВОЗИТЬ СВІЙ ВІДЛІК. Відмова STR_OZ_LOCK_TOO_MANY несе
        // тіло -- OZ_PdaDeviceStatus, де заповнені лише LockedOut і
        // LockWaitS (решта полів -- умовчання, і читати їх не можна). Так
        // само відповідає й остання хибна спроба, що щойно стала
        // блокуванням. Тіла немає чи воно не читається -- причина без числа.
        if (error == "STR_OZ_LOCK_TOO_MANY")
        {
            int waitS = 0;
            if (json != "")
            {
                string err;
                OZ_PdaDeviceStatus lo = new OZ_PdaDeviceStatus();
                if (JsonFileLoader<OZ_PdaDeviceStatus>.LoadData(json, lo, err))
                    waitS = lo.LockWaitS;
            }

            ArmLockout(waitS);
            PaintPinPrompt("");
            return;
        }

        // Будь-яка інша відповідь означає, що спробу розглянули або
        // відмовили з іншої причини -- відлік, якщо й був, уже не правда.
        // STR_OZ_ERR_SEALED лишається зі своїм текстом.
        DisarmLockout();

        string why = "#STR_OZ_LOCK_WRONG";
        if (error != "")
            why = "#" + error;
        PaintPinPrompt(why);
    }

    // «Назад»: крок назад із екрана коду, інакше -- геть із КПК.
    //
    // Одне тіло на два шляхи -- опит UAUIBack і подія клавіші, -- і саме тому
    // воно ОДНЕ: шляхи бачать те саме натискання, і якби кожен вирішував сам,
    // добровільний екран коду скасувався б РАЗОМ із закриттям вікна. Другий
    // виклик у тому ж натисканні відсікає позначка часу нижче.
    private int m_BackAtMs = 0;

    private void Back()
    {
        int now = GetGame().GetTime();
        // 200 мс -- це менше за будь-яке подвійне натискання людиною й більше
        // за будь-який розрив між кадром і подією клавіші.
        if (now - m_BackAtMs < 200)
            return;
        m_BackAtMs = now;

        // Добровільний екран коду скасовується; примусовий -- ні, бо з
        // замкненого пристрою виходити нема куди, крім як із меню.
        if (m_PinMode != "" && m_PinMode != "unlock")
        {
            EndPin();
            return;
        }

        Close();
    }

    // Кадровий опит «назад». Дешевий: один прапорець рушія на кадр, поки
    // вікно відкрите. ГОЛОВНИЙ шлях: він єдиний працює, коли клавіатуру
    // тримає поле вводу.
    override void Update(float timeslice)
    {
        super.Update(timeslice);

        if (!m_BackInput)
            return;
        if (m_BackInput.InputP().LocalPress())
            Back();
    }

    override bool OnKeyPress(Widget w, int x, int y, int key)
    {
        // Escape лишається ЗАПАСНИМ шляхом -- на випадок, якщо опит UAUIBack
        // мовчить (чужі виключення входу, перепризначення). Коли працюють
        // обидва, друге спрацювання з'їдає позначка часу в Back().
        if (key == KeyCode.KC_ESCAPE)
        {
            Back();
            return true;
        }

        // ЦИФР КОДУ ТУТ БІЛЬШЕ НЕМАЄ -- вони приходять через OnPinKey.
        return super.OnKeyPress(w, x, y, key);
    }

    // КОД З КЛАВІАТУРИ -- З ГЛОБАЛЬНОГО ШЛЯХУ, а не з OnKeyPress меню.
    //
    // OnKeyPress(Widget ...) -- подія ВІДЖЕТА з фокусом, і на екрані коду її
    // не отримував ніхто: зміряно на стенді 2026-09-28 -- фокус на корені
    // (OnShow) чи на кнопці пада після справжнього кліку мишею, а цифри,
    // подані в клієнт скан-кодами через SendInput (так само, як їх подає
    // клавіатура), у пад не падали жодного разу; підказка ж обіцяла саме
    // клавіатуру. Сире натискання рушій віддає DayZGame.OnKeyPress ->
    // Mission.OnKeyPress(int) незалежно від фокуса, і звідти його сюди
    // передає OZ_PdaMissionGameplay. Шлях ОДИН: друга копія цієї логіки в
    // OnKeyPress меню набирала б цифру двічі там, де спрацювали б обидва.
    void OnPinKey(int key)
    {
        // Код набирається лише поки відкритий екран коду -- і не на
        // запечатаному приладі: там пад схований (PaintSealed), бо коду
        // ніхто не знає, і клавіатура не мусить набирати того, чого не
        // набирає миша.
        if (m_PinMode == "" || m_Sealed)
            return;

        // ТА САМА ДОВЖИНА, ЩО В ПАДА Й У КРАПКАХ (ТЗ-5 R-B3.3). Тут
        // лишалася четвірка літералом, і з профілем на шість цифр клавіатура
        // спинялась на четвертій: пад малював шість рисок, мишею шість цифр
        // набиралось, а Enter слав чотири -- і сервер відмовляв за довжиною
        // (OZ_PDA_Base.PinShaped). Підказка STR_OZ_PIN_HINT кличе саме на
        // клавіатуру.
        string digit = KeyDigit(key);
        if (digit != "")
        {
            if (m_PinBuffer.Length() < PadLength())
            {
                m_PinBuffer += digit;
                PaintPinDots();
            }
            return;
        }

        if (key == KeyCode.KC_BACK && m_PinBuffer.Length() > 0)
        {
            m_PinBuffer = m_PinBuffer.Substring(0, m_PinBuffer.Length() - 1);
            PaintPinDots();
            return;
        }

        // Enter цифрового блоку -- той самий Enter: хто набирає код
        // праворуч, підтверджує там само.
        bool enter = false;
        if (key == KeyCode.KC_RETURN || key == KeyCode.KC_NUMPADENTER)
            enter = true;
        if (!enter)
            return;

        // Відмітка -- на КОЖЕН Enter, і з порожнім буфером теж: луна від
        // рушія (див. EnterEcho) приходить однаково.
        m_KeyEnterAtMs = GetGame().GetTime();
        if (m_PinBuffer.Length() > 0)
            PinConfirm();
    }

    // ЛУНА ENTER. Рушій на Enter сам «клікає» кнопку, над якою стоїть
    // курсор, -- зміряно на стенді 2026-09-28: порожній пад, один Enter без
    // жодної цифри, і в буфері з'явилась «5»: меню відкривається з курсором
    // посередині, а посередині пада саме Key5. Разом із підтвердженням з
    // клавіатури це давало зайву цифру на наступному кроці («повторіть код»
    // починався з «5»), над OK -- другу спробу того самого коду (одна
    // помилка рахувалась би двічі), а над FACTORY RESET -- скидання. Тож
    // поки відкритий екран коду, клік одразу після Enter -- це луна, а не
    // рука гравця.
    private static const int ENTER_ECHO_MS = 500;
    private int m_KeyEnterAtMs = -1;

    private bool EnterEcho()
    {
        if (m_KeyEnterAtMs < 0)
            return false;
        return GetGame().GetTime() - m_KeyEnterAtMs < ENTER_ECHO_MS;
    }

    // Цифра клавіші або порожньо.
    //
    // ПОІМЕННО, а не відніманням. У KeyCode (1_core/proto/ensystem.c) нуль
    // стоїть ПІСЛЯ дев'ятки -- KC_1..KC_9, а тоді KC_0, -- тож умова
    // «key >= KC_0 && key <= KC_9» не справджувалась ніколи, і набір коду з
    // клавіатури був мертвий; «key - KC_0» до того ж давав би не ту цифру.
    // Цифровий блок -- окремий ряд (KC_NUMPAD7..9, 4..6, 1..3, 0 упереміш зі
    // знаками), і його теж треба назвати.
    private string KeyDigit(int key)
    {
        if (key == KeyCode.KC_1 || key == KeyCode.KC_NUMPAD1)
            return "1";
        if (key == KeyCode.KC_2 || key == KeyCode.KC_NUMPAD2)
            return "2";
        if (key == KeyCode.KC_3 || key == KeyCode.KC_NUMPAD3)
            return "3";
        if (key == KeyCode.KC_4 || key == KeyCode.KC_NUMPAD4)
            return "4";
        if (key == KeyCode.KC_5 || key == KeyCode.KC_NUMPAD5)
            return "5";
        if (key == KeyCode.KC_6 || key == KeyCode.KC_NUMPAD6)
            return "6";
        if (key == KeyCode.KC_7 || key == KeyCode.KC_NUMPAD7)
            return "7";
        if (key == KeyCode.KC_8 || key == KeyCode.KC_NUMPAD8)
            return "8";
        if (key == KeyCode.KC_9 || key == KeyCode.KC_NUMPAD9)
            return "9";
        if (key == KeyCode.KC_0 || key == KeyCode.KC_NUMPAD0)
            return "0";
        return "";
    }

    // Зміна в полі вводу -- сторінці, яка його тримає. Досі сторінки не
    // чули змін узагалі, і лічильник байтів (ТЗ-4 R-D1.3) було нікуди
    // повісити.
    override bool OnChange(Widget w, int x, int y, bool finished)
    {
        if (m_Current != "" && m_Pages && m_Pages.Contains(m_Current))
        {
            OZ_PdaPage page = m_Pages.Get(m_Current);
            if (page && page.OnPageChange(w, finished))
                return true;
        }
        return super.OnChange(w, x, y, finished);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        // Першим рядком, до будь-якої кнопки: луна Enter влучає в те, що під
        // курсором, -- зокрема в FACTORY RESET екрана коду (див. EnterEcho).
        if (m_PinMode != "" && EnterEcho())
            return true;

        if (w == m_BtnClose)
        {
            Close();
            return true;
        }

        if (w && w.GetUserID() == 1)
        {
            Select(w.GetName());
            return true;
        }

        if (m_PinMode != "" && w && w.GetName() == "BtnCrack")
        {
            OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "crack", "{}");
            return true;
        }

        if (w && w.GetName() == "BtnInitBig")
        {
            OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "initiate", "{}");
            return true;
        }

        if (m_PinMode != "" && w && w.GetName() == "BtnFactory")
        {
            OZ_Rpc.Request(OZ_PdaConst.PAGE_DEVICE, "factory_reset", "{}");
            return true;
        }

        // Цифрова панель екрана коду. Розбирається ТУТ, а не на сторінці:
        // набраний код живе в меню й ніде більше.
        if (m_PinMode != "" && w && PinPadClick(w.GetName()))
            return true;

        // Далі -- активна сторінка й та, що ділить із нею вкладку. Решту не
        // питаємо: сторінки, якої не видно, клікнути неможливо, і давати їй
        // голос означало б ловити чужі кнопки.
        if (m_Current != "" && m_Pages.Contains(m_Current))
        {
            if (m_Pages.Get(m_Current).OnPageClick(w, x, y))
                return true;
        }

        OZ_PdaPage mate = Companion();
        if (mate && mate.OnPageClick(w, x, y))
            return true;

        return super.OnClick(w, x, y, button);
    }

    // ПОДВІЙНИЙ клік їде на сторінку тим самим шляхом, що й одинарний, і
    // тими самими двома адресатами. Подія окрема (enwidgets.c:660), і меню
    // ловить її для БУДЬ-ЯКОГО свого віджета: подія, яку обробник не спожив,
    // піднімається батьками до кореня розкладки, а обробник кореня -- меню
    // (ваніль робить так само: консоль розробника ловить подвійний клік по
    // списках і по мапі саме на рівні меню, scriptconsole.c:332).
    //
    // Кнопку фільтруємо так само, як на відпусканні: двічі правою -- це не
    // подвійний клік лівою (ідіом ванілі, serverbrowserentry.c:140).
    override bool OnDoubleClick(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && m_Current != "" && m_Pages.Contains(m_Current))
        {
            if (m_Pages.Get(m_Current).OnPageDoubleClick(w, x, y))
                return true;

            OZ_PdaPage twin = Companion();
            if (twin && twin.OnPageDoubleClick(w, x, y))
                return true;
        }

        return super.OnDoubleClick(w, x, y, button);
    }

    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        if (m_Current != "" && m_Pages.Contains(m_Current))
        {
            if (m_Pages.Get(m_Current).OnPageMouseDown(w, x, y))
                return true;
        }

        OZ_PdaPage mate = Companion();
        if (mate && mate.OnPageMouseDown(w, x, y))
            return true;

        return super.OnMouseButtonDown(w, x, y, button);
    }

    // Відпускання лівої так само їде на сторінку: клік по MapWidget не
    // породжує OnClick, і карта збирає його з пари down+up сама.
    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && m_Current != "" && m_Pages.Contains(m_Current))
        {
            if (m_Pages.Get(m_Current).OnPageMouseUp(w, x, y))
                return true;

            OZ_PdaPage mate = Companion();
            if (mate && mate.OnPageMouseUp(w, x, y))
                return true;
        }

        return super.OnMouseButtonUp(w, x, y, button);
    }

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (m_Current != "" && m_Pages.Contains(m_Current))
        {
            if (m_Pages.Get(m_Current).OnPageItemSelected(w, row))
                return true;
        }

        OZ_PdaPage mate = Companion();
        if (mate && mate.OnPageItemSelected(w, row))
            return true;

        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnMouseEnter(Widget w, int x, int y)
    {
        if (w && w.GetUserID() == 1)
        {
            Widget hover = w.FindAnyWidget("TabHover");
            if (hover)
                hover.Show(true);
            return true;
        }
        return super.OnMouseEnter(w, x, y);
    }

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        if (w && w.GetUserID() == 1)
        {
            Widget hover = w.FindAnyWidget("TabHover");
            if (hover)
                hover.Show(false);
            return true;
        }
        return super.OnMouseLeave(w, enterW, x, y);
    }
}

// Тонкий перехідник: ядро кличе слухача, слухач кличе меню. Меню не може
// успадкувати OZ_ResponseListener саме, бо воно вже UIScriptedMenu.
class OZ_PdaMenuListener : OZ_ResponseListener
{
    private OZ_PdaMenu m_Menu;

    void OZ_PdaMenuListener(OZ_PdaMenu menu)
    {
        m_Menu = menu;
    }

    override void OnResponse(string pageId, string op, bool ok, string json, string error)
    {
        if (m_Menu)
            m_Menu.HandleResponse(pageId, op, ok, json, error);
    }
}
