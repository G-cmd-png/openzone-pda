// Панель «PDA» в адмінському вікні OpenZone: форма Tuning.json полями,
// праворуч -- міні-вкладки HARDWARE (форма модулів і носіїв) та PROFILES
// (сирий JSON). Чіпляється вкладкою до вікна ядра через modded class --
// «субмод субмода»: ядро про КПК не знає, КПК доклада свою вкладку сам.
//
// Гарди: NO_GUI -- сервер компілює Mission без UI; AVPPAdminTools і
// OpenZone_VPP -- імена класів CfgMods (їх авто-дефайнить рушій).

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    private ref OZ_PdaTuning m_PdaTun;
    private ref OZ_PdaHardwareConfig m_HwCfg;

    // Рядок списку заліза -> (вид, індекс у своєму масиві).
    private ref array<string> m_HwRowKind;
    private ref array<int>    m_HwRowIdx;
    private string m_HwPickedKind = "";
    private int    m_HwPickedIdx = -1;
    private bool   m_HwNewMode = false;
    private bool   m_HwWritable = true;
    private bool   m_HwDelArmed = false;

    // Редактор Profiles.json утримав НЕ ВЕСЬ файл (див. OnCfgText) -- тоді
    // APPLY вимкнений: він надіслав би на сервер обрубок.
    private bool   m_ProfCut = false;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);

        if (!M_SUB_WIDGET)
            return;

        m_HwRowKind = new array<string>();
        m_HwRowIdx  = new array<int>();

        Widget pane = GetGame().GetWorkspace().CreateWidgets("OpenZone_PDA_VPP/gui/layouts/oz_pda_vpp_pane.layout", M_SUB_WIDGET);
        if (!pane)
        {
            OZ_Log.Error("pda vpp pane: layout failed to load");
            return;
        }

        // Ім'я рядка підказок називає САМА панель. Без нього ядро мусило б
        // тримати перелік чужих імен віджетів і вгадувати наш -- перебір,
        // який панель рації вже проходила мимо.
        RegisterPane("pda", "PDA", pane, "PdaHint");
    }

    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);

        if (id == "pda")
        {
            AskCfg("Tuning");
            AskCfg("Hardware");
        }
    }

    override void OnCfgText(string name, string body)
    {
        if (name == "Tuning")
        {
            OZ_PdaTuning t = new OZ_PdaTuning();
            string err;
            if (JsonFileLoader<OZ_PdaTuning>.LoadData(body, t, err) && t)
            {
                m_PdaTun = t;
                FillTuningForm();
            }
            else
                Hint("Tuning.json does not parse: " + err);
            return;
        }

        if (name == "Hardware")
        {
            OZ_PdaHardwareConfig hc = new OZ_PdaHardwareConfig();
            string herr;
            if (JsonFileLoader<OZ_PdaHardwareConfig>.LoadData(body, hc, herr) && hc)
            {
                // Корінь створив скрипт, а от кожен запис у двох списках --
                // серіалізатор, і адмін правитиме їх ще довго, перш ніж
                // натисне «зберегти» (шапка OZ_ConfigBase ядра). Пересідаємо
                // одразу, поки читання чесне; Validate тут не кличемо -- це
                // редактор, а не завантаження, і скарги в лог не його справа.
                int hi;
                for (hi = 0; hc.Modules && hi < hc.Modules.Count(); hi++)
                {
                    if (hc.Modules[hi])
                        hc.Modules.Set(hi, hc.Modules[hi].Copy());
                }
                for (hi = 0; hc.Carriers && hi < hc.Carriers.Count(); hi++)
                {
                    if (hc.Carriers[hi])
                        hc.Carriers.Set(hi, hc.Carriers[hi].Copy());
                }

                m_HwCfg = hc;
                RebuildHwList();
            }
            else
            {
                Hint("Hardware.json does not parse: " + herr);
            }
            return;
        }

        if (name == "Profiles")
        {
            MultilineEditBoxWidget ed = MultilineEditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("PdaProfEdit"));
            if (!ed)
                return;
            ed.SetText(body);

            // РЕДАКТОР МІРЯЄ СЕБЕ САМ, а не вірить, що вмістив.
            //
            // Поле вводу тримає обмежену кількість байтів (скіл, gui-layouts
            // §22: вставка на 1371 байт лишилась рівно 512), а Profiles.json
            // живого сервера -- кілька тисяч. Досі APPLY слав назад те, що
            // поле ВТРИМАЛО, тобто обрубок, і сервер або відмовляв (JSON не
            // розбирається), або -- гірше -- приймав урізаний перелік
            // профілів. Тепер порівнюємо довжину того, що поле віддає, з
            // довжиною файла: менше -- APPLY вимкнений до наступного
            // перечитування, а адмін знає, що файл треба правити на диску.
            string got;
            ed.GetText(got);
            bool cut = false;
            if (got.Length() < body.Length())
                cut = true;
            m_ProfCut = cut;

            if (cut)
                Hint("Profiles.json is " + body.Length().ToString() + " bytes, this editor holds " + got.Length().ToString() + ": APPLY is off, edit the file on disk");
            else
                Hint("Profiles loaded");
            return;
        }

        super.OnCfgText(name, body);
    }

    override void OnCfgApplied()
    {
        super.OnCfgApplied();
        if (CurrentPane() == "pda")
        {
            AskCfg("Tuning");
            AskCfg("Hardware");
        }
    }

    // ---------------------------------------------------------- тюнінг

    // СІМНАДЦЯТЬ ПОЛІВ ІЗ ВІСІМНАДЦЯТИ, і вісімнадцяте названо тут навмисно.
    //
    // Tun_GroupInviteTtlSeconds форми не має: колонка тюнінгу вже стоїть
    // рівно по нижній край панелі (арифметика -- у примітці опису
    // ui/OpenZone_PDA_VPP/oz_pda_vpp_pane.json), і ще один рядок виштовхнув
    // би її за межі. Редагується воно вкладкою CONFIG адмінського вікна --
    // тим самим сирим JSON, яким редагуються Profiles.
    private void FillTuningForm()
    {
        if (!m_PdaTun)
            return;

        SetEdit("Tun_PinMaxFails",         m_PdaTun.PinMaxFails.ToString());
        SetEdit("Tun_PinLockoutSeconds",   m_PdaTun.PinLockoutSeconds.ToString());
        SetEdit("Tun_ChatMsgMaxBytes",     m_PdaTun.ChatMsgMaxBytes.ToString());
        SetEdit("Tun_ChatTitleMaxBytes",   m_PdaTun.ChatTitleMaxBytes.ToString());
        SetEdit("Tun_ChatDescMaxBytes",    m_PdaTun.ChatDescMaxBytes.ToString());
        SetEdit("Tun_ChatHistoryOpen",     m_PdaTun.ChatHistoryOpen.ToString());
        SetEdit("Tun_ChatHistoryPage",     m_PdaTun.ChatHistoryPage.ToString());
        SetEdit("Tun_ChatGroupMax",        m_PdaTun.ChatGroupMax.ToString());
        SetEdit("Tun_NoteTitleMaxBytes",   m_PdaTun.NoteTitleMaxBytes.ToString());
        SetEdit("Tun_NoteBodyMaxBytes",    m_PdaTun.NoteBodyMaxBytes.ToString());
        SetEdit("Tun_MarkerNameMaxBytes",  m_PdaTun.MarkerNameMaxBytes.ToString());
        SetEdit("Tun_MarkerDescMaxBytes",  m_PdaTun.MarkerDescMaxBytes.ToString());
        SetEdit("Tun_FriendReachMeters",   m_PdaTun.FriendReachMeters.ToString());
        SetEdit("Tun_SwapOfferTtlSeconds", m_PdaTun.SwapOfferTtlSeconds.ToString());
        SetEdit("Tun_ToastSeconds",        m_PdaTun.ToastSeconds.ToString());
        SetEdit("Tun_RouteAdvanceMeters",  m_PdaTun.RouteAdvanceMeters.ToString());
        SetEdit("Tun_BeaconPushSeconds",   m_PdaTun.BeaconPushSeconds.ToString());
    }

    private void SaveTuningForm()
    {
        if (!m_PdaTun)
        {
            Hint("Tuning is not loaded yet");
            return;
        }

        // Правимо ЗАВАНТАЖЕНИЙ об'єкт: Version і майбутні поля, яких форма
        // не знає, переживають збереження недоторканими.
        m_PdaTun.PinMaxFails         = GetEdit("Tun_PinMaxFails").ToInt();
        m_PdaTun.PinLockoutSeconds   = GetEdit("Tun_PinLockoutSeconds").ToInt();
        m_PdaTun.ChatMsgMaxBytes     = GetEdit("Tun_ChatMsgMaxBytes").ToInt();
        m_PdaTun.ChatTitleMaxBytes   = GetEdit("Tun_ChatTitleMaxBytes").ToInt();
        m_PdaTun.ChatDescMaxBytes    = GetEdit("Tun_ChatDescMaxBytes").ToInt();
        m_PdaTun.ChatHistoryOpen     = GetEdit("Tun_ChatHistoryOpen").ToInt();
        m_PdaTun.ChatHistoryPage     = GetEdit("Tun_ChatHistoryPage").ToInt();
        m_PdaTun.ChatGroupMax        = GetEdit("Tun_ChatGroupMax").ToInt();
        m_PdaTun.NoteTitleMaxBytes   = GetEdit("Tun_NoteTitleMaxBytes").ToInt();
        m_PdaTun.NoteBodyMaxBytes    = GetEdit("Tun_NoteBodyMaxBytes").ToInt();
        m_PdaTun.MarkerNameMaxBytes  = GetEdit("Tun_MarkerNameMaxBytes").ToInt();
        m_PdaTun.MarkerDescMaxBytes  = GetEdit("Tun_MarkerDescMaxBytes").ToInt();
        m_PdaTun.FriendReachMeters   = GetEdit("Tun_FriendReachMeters").ToInt();
        m_PdaTun.SwapOfferTtlSeconds = GetEdit("Tun_SwapOfferTtlSeconds").ToInt();
        m_PdaTun.ToastSeconds        = GetEdit("Tun_ToastSeconds").ToInt();
        m_PdaTun.RouteAdvanceMeters  = GetEdit("Tun_RouteAdvanceMeters").ToInt();
        m_PdaTun.BeaconPushSeconds   = GetEdit("Tun_BeaconPushSeconds").ToInt();

        string body;
        string err;
        if (!JsonFileLoader<OZ_PdaTuning>.MakeData(m_PdaTun, body, err, false))
        {
            Hint("cannot serialise");
            return;
        }
        SendCfg("Tuning", body);
    }

    // ---------------------------------------------------------- залізо

    private void RebuildHwList()
    {
        TextListboxWidget lb = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("HwList"));
        if (!lb || !m_HwCfg)
            return;

        lb.ClearItems();
        m_HwRowKind.Clear();
        m_HwRowIdx.Clear();

        if (m_HwCfg.Modules)
        {
            for (int i = 0; i < m_HwCfg.Modules.Count(); i++)
            {
                lb.AddItem("M  " + m_HwCfg.Modules[i].ClassName, NULL, 0);
                m_HwRowKind.Insert("module");
                m_HwRowIdx.Insert(i);
            }
        }

        if (m_HwCfg.Carriers)
        {
            for (int c = 0; c < m_HwCfg.Carriers.Count(); c++)
            {
                lb.AddItem("C  " + m_HwCfg.Carriers[c].ClassName, NULL, 0);
                m_HwRowKind.Insert("carrier");
                m_HwRowIdx.Insert(c);
            }
        }
    }

    private void FillHwForm(string kind, int idx)
    {
        m_HwPickedKind = kind;
        m_HwPickedIdx  = idx;
        m_HwNewMode    = false;
        m_HwDelArmed   = false;

        if (kind == "module")
        {
            OZ_ModuleSpec ms = m_HwCfg.Modules[idx];
            SetEdit("HwClass", ms.ClassName);
            SetEdit("HwName",  ms.DisplayName);
            SetEdit("HwKind",  ms.Kind);
            SetEdit("HwRange", ms.RangeM.ToString());
            SetEdit("HwPower", ms.PowerFactor.ToString());
            SetEdit("HwSpy",   ms.SpyMinutes.ToString());
            SetEdit("HwPages", JoinPages(ms.EnablesPages));
            SetEdit("HwMarks", "");
            m_HwWritable = true;

            // Запис, який оголосив чужий мод, адмін бачить як такий: SAVE
            // зробить його своїм, а DELETE не допоможе (див. нижче).
            if (ms.Origin != "")
            {
                PaintHwToggles();
                Hint("module: " + ms.ClassName + " (declared by a mod; SAVE makes it yours)");
                return;
            }
        }
        else
        {
            OZ_CarrierSpec cs = m_HwCfg.Carriers[idx];
            SetEdit("HwClass", cs.ClassName);
            SetEdit("HwName",  cs.DisplayName);
            // Носій більше не має "виду вмісту": на ньому лежить стільки родів,
            // скільки на нього записали.
            SetEdit("HwKind",  "");
            SetEdit("HwRange", "");
            SetEdit("HwPower", "");
            SetEdit("HwSpy",   "");
            SetEdit("HwPages", "");
            // Одне число замість двох: місткість носія тепер рахується в
            // ЗАПИСАХ, байдуже яких. Див. OZ_CarrierSpec.
            SetEdit("HwMarks", cs.MaxRecords.ToString());
            m_HwWritable = cs.Writable;
        }
        PaintHwToggles();
        Hint(kind + ": " + GetEdit("HwClass"));
    }

    private void NewHwForm(string kind)
    {
        m_HwPickedKind = kind;
        m_HwPickedIdx  = -1;
        m_HwNewMode    = true;
        m_HwDelArmed   = false;

        SetEdit("HwClass", "");
        SetEdit("HwName",  "");
        SetEdit("HwRange", "0");
        SetEdit("HwPower", "1.0");
        SetEdit("HwSpy",   "0");
        SetEdit("HwPages", "");
        SetEdit("HwMarks", "0");
        // Носій виду не має ЗОВСІМ -- на ньому лежить стільки родів, скільки
        // на нього записали, і SaveHwForm поле для нього не читає. Слово
        // "markers" тут було залишком старої моделі й брехало адмінові.
        // Вид ПІДКАЗУЄМО, а не вгадуємо: порожнє поле Validate зустріне
        // скаргою «модуль причепиться й не робитиме нічого», а вписати сюди
        // "gps" означало б підштовхнути адміна до другого приймача. Слів
        // "antenna", "radiometer" і "dosimeter" тут більше немає -- цих видів
        // не існує з 2026-09-09.
        //
        // Підказка -- ДЕШИФРАТОР: єдиний вид, якого в приладі може не бути й
        // від другого примірника якого нічого не ламається.
        if (kind == "module")
            SetEdit("HwKind", OZ_PdaConst.MOD_DECRYPTOR);
        else
            SetEdit("HwKind", "");
        m_HwWritable = true;
        PaintHwToggles();
        if (kind == "module")
            Hint("new module: classname is the key, Kind is gps/decryptor/spy or a foreign one, then SAVE");
        else
            Hint("new " + kind + ": classname is the key, then SAVE");
    }

    private void PaintHwToggles()
    {
        TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnHwWritableText"));
        if (!t)
            return;
        if (m_HwWritable)
            t.SetText("writable: yes");
        else
            t.SetText("writable: no");
    }

    private void SaveHwForm()
    {
        if (!m_HwCfg)
        {
            Hint("Hardware is not loaded yet");
            return;
        }

        string cls = GetEdit("HwClass");
        if (cls == "" || cls.IndexOf(" ") != -1)
        {
            Hint("classname must be a single word");
            return;
        }

        if (m_HwPickedKind == "module")
        {
            OZ_ModuleSpec ms;
            if (m_HwNewMode)
            {
                ms = new OZ_ModuleSpec();
                if (!m_HwCfg.Modules)
                    m_HwCfg.Modules = new array<ref OZ_ModuleSpec>();
                m_HwCfg.Modules.Insert(ms);
                m_HwPickedIdx = m_HwCfg.Modules.Count() - 1;
                m_HwNewMode = false;
            }
            else
            {
                if (m_HwPickedIdx < 0 || m_HwPickedIdx >= m_HwCfg.Modules.Count())
                {
                    Hint("pick an entry first");
                    return;
                }
                ms = m_HwCfg.Modules[m_HwPickedIdx];
            }

            ms.ClassName   = cls;
            ms.DisplayName = GetEdit("HwName");
            ms.Kind        = GetEdit("HwKind");
            ms.RangeM      = GetEdit("HwRange").ToFloat();
            ms.PowerFactor = GetEdit("HwPower").ToFloat();
            ms.SpyMinutes  = GetEdit("HwSpy").ToFloat();
            ms.EnablesPages = SplitPages(GetEdit("HwPages"));

            // ПРАВКА АДМІНА ЗАБИРАЄ ЗАПИС СОБІ.
            //
            // Запис із непорожнім Origin належить модові, і кожне
            // завантаження накладає його оголошення знову (OZ_PdaHardware.
            // Insert чіпає лише записи модів). Гаряче застосування -- теж
            // завантаження, тож SAVE тут був холостим: форма казала
            // «збережено», а ServerLoad за мить повертав модові значення.
            // Порожній Origin -- це й є «запис адміна»: так обіцяє коментар
            // над полем Origin, і тепер це робить сама форма.
            if (ms.Origin != "")
                ms.Origin = "";
        }
        else if (m_HwPickedKind == "carrier")
        {
            OZ_CarrierSpec cs;
            if (m_HwNewMode)
            {
                cs = new OZ_CarrierSpec();
                if (!m_HwCfg.Carriers)
                    m_HwCfg.Carriers = new array<ref OZ_CarrierSpec>();
                m_HwCfg.Carriers.Insert(cs);
                m_HwPickedIdx = m_HwCfg.Carriers.Count() - 1;
                m_HwNewMode = false;
            }
            else
            {
                if (m_HwPickedIdx < 0 || m_HwPickedIdx >= m_HwCfg.Carriers.Count())
                {
                    Hint("pick an entry first");
                    return;
                }
                cs = m_HwCfg.Carriers[m_HwPickedIdx];
            }

            cs.ClassName   = cls;
            cs.DisplayName = GetEdit("HwName");
            cs.Writable    = m_HwWritable;
            cs.MaxRecords  = GetEdit("HwMarks").ToInt();
        }
        else
        {
            Hint("pick an entry or press NEW first");
            return;
        }

        PushHwCfg();
    }

    private void DeleteHwEntry()
    {
        if (!m_HwCfg || m_HwPickedIdx < 0 || m_HwNewMode)
        {
            Hint("pick an entry first");
            return;
        }

        // МОДУЛЬ ЧУЖОГО МОДА НЕ ВИДАЛЯЄТЬСЯ -- ВІН ПОВЕРТАЄТЬСЯ. Мод оголошує
        // його з кожним завантаженням конфіга, і гаряче застосування цього
        // DELETE саме таким завантаженням і є: рядок зникав зі списку й за
        // мить вертався, а адмін не знав чому. Вимкнути його можна правкою:
        // SAVE робить запис адміновим, і далі мод його не чіпає.
        if (m_HwPickedKind == "module" && m_HwPickedIdx < m_HwCfg.Modules.Count())
        {
            OZ_ModuleSpec picked = m_HwCfg.Modules[m_HwPickedIdx];
            if (picked && picked.Origin != "")
            {
                m_HwDelArmed = false;
                Hint("declared by a mod: it comes back on every load; edit it and SAVE instead");
                return;
            }
        }

        if (!m_HwDelArmed)
        {
            m_HwDelArmed = true;
            Hint("press DELETE again to remove " + GetEdit("HwClass"));
            return;
        }
        m_HwDelArmed = false;

        // RemoveOrdered: Remove міняє місцями з останнім, і файл, який адмін
        // потім читає очима, переставлявся б від кожного видалення.
        if (m_HwPickedKind == "module")
            m_HwCfg.Modules.RemoveOrdered(m_HwPickedIdx);
        else
            m_HwCfg.Carriers.RemoveOrdered(m_HwPickedIdx);

        m_HwPickedIdx = -1;
        m_HwPickedKind = "";
        PushHwCfg();
    }

    private void PushHwCfg()
    {
        string body;
        string err;
        if (!JsonFileLoader<OZ_PdaHardwareConfig>.MakeData(m_HwCfg, body, err, false))
        {
            Hint("cannot serialise");
            return;
        }
        SendCfg("Hardware", body);
    }

    // ПЕРЕНОС УСЕРЕДИНІ РЯДКОВОГО ЗНАЧЕННЯ -- ЦЕ ПЕРЕНОС РЕДАКТОРА.
    //
    // Поле з `lines` переносить набране, ВСТАВЛЯЮЧИ справжній \n (скіл,
    // gui-layouts §22), і в Profiles.json він падав посеред id сторінки чи
    // класнейма: "contac\nts" -- уже інша сторінка, і жоден прилад її не
    // знайде. Сирий перенос усередині рядка JSON заборонений, а між токенами
    // він -- звичайний пробіл, тож викидаємо лише ті, що стоять у лапках.
    // Ріжемо по \n і \r (ASCII): кожен шматок лишається цілим UTF-8.
    private string DropWrapsInStrings(string text)
    {
        string outp = "";
        int n = text.Length();
        int from = 0;
        bool inStr = false;
        bool esc = false;

        for (int i = 0; i < n; i++)
        {
            string ch = text.Substring(i, 1);
            if (esc)
            {
                esc = false;
                continue;
            }
            if (inStr && ch == "\\")
            {
                esc = true;
                continue;
            }
            if (ch == "\"")
            {
                inStr = !inStr;
                continue;
            }
            if (inStr && (ch == "\n" || ch == "\r"))
            {
                if (i > from)
                    outp += text.Substring(from, i - from);
                from = i + 1;
            }
        }

        if (from < n)
            outp += text.Substring(from, n - from);
        return outp;
    }

    private string JoinPages(array<string> pages)
    {
        if (!pages)
            return "";
        string outp = "";
        for (int i = 0; i < pages.Count(); i++)
        {
            if (outp != "")
                outp += " ";
            outp += pages[i];
        }
        return outp;
    }

    private ref array<string> SplitPages(string text)
    {
        array<string> outp = new array<string>();
        array<string> parts = new array<string>();
        text.Split(" ", parts);
        for (int i = 0; i < parts.Count(); i++)
        {
            if (parts[i] != "")
                outp.Insert(parts[i]);
        }
        return outp;
    }

    // ---------------------------------------------------------- ввід

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && w.GetName() == "HwList")
        {
            if (row >= 0 && row < m_HwRowKind.Count())
                FillHwForm(m_HwRowKind[row], m_HwRowIdx[row]);
            return true;
        }
        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            string nm = w.GetName();

            if (nm == "BtnTunSave")
            {
                SaveTuningForm();
                return true;
            }

            if (nm == "BtnPdaSubHw" || nm == "BtnPdaSubProf")
            {
                Widget hw = M_SUB_WIDGET.FindAnyWidget("PdaHwBox");
                Widget pf = M_SUB_WIDGET.FindAnyWidget("PdaProfBox");
                bool wantHw = (nm == "BtnPdaSubHw");
                if (hw)
                    hw.Show(wantHw);
                if (pf)
                    pf.Show(!wantHw);
                if (!wantHw)
                    AskCfg("Profiles");
                return true;
            }

            if (nm == "BtnHwSave")
            {
                SaveHwForm();
                return true;
            }

            if (nm == "BtnHwDel")
            {
                DeleteHwEntry();
                return true;
            }

            if (nm == "BtnHwNewMod")
            {
                NewHwForm("module");
                return true;
            }

            if (nm == "BtnHwNewCar")
            {
                NewHwForm("carrier");
                return true;
            }

            if (nm == "BtnHwWritable")
            {
                m_HwWritable = !m_HwWritable;
                PaintHwToggles();
                return true;
            }

            if (nm == "BtnProfReload")
            {
                AskCfg("Profiles");
                return true;
            }

            if (nm == "BtnProfApply")
            {
                if (m_ProfCut)
                {
                    Hint("the editor holds only part of Profiles.json: APPLY would cut the file, edit it on disk");
                    return true;
                }

                MultilineEditBoxWidget ed = MultilineEditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("PdaProfEdit"));
                if (ed)
                {
                    string body;
                    ed.GetText(body);
                    SendCfg("Profiles", DropWrapsInStrings(body));
                }
                return true;
            }
        }

        return super.OnClick(w, x, y, button);
    }
}

#endif
#endif
#endif
