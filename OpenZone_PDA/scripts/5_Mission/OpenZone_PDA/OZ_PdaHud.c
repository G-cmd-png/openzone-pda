// HUD КПК: тост чату й мінімапа.
//
// НЕ ШЛЕ НІЧОГО. Мінімапа живе з того, що вже на клієнті
// (мережеві змінні предмета й позиція гравця), а тост -- з пушів, які
// сервер і так шле кожному одержувачу рядка (OZ_ChatSink). Жодного
// опитування сервера звідси немає й не буде.
//
// АКТИВНІСТЬ ОВЕРЛЕЇВ: ЛИШЕ СЛОТ НОСІННЯ (ТЗ-5 R-B1.1). Прилад у руках
// оверлеїв не дає -- ані тоста, ані мінікарти; підібраний чужий КПК теж
// (R-B1.3). Екран його при цьому відкривається як раніше: то інше
// правило й інший резолвер (OZ_PdaLookup.HeldByPlayer, тут -- Device()).
// У рюкзаку прилад мовчить у ОБОХ правилах -- рішення власника
// 2026-08-28, і воно ж зробило пошук дешевим: вибірка O(1) замість
// обходу інвентаря.

// Вуха HUD: окремий екземпляр для підписки на ScriptInvoker. Справжня
// пастка була НЕ тут, а в місці озброєння: підписка жила в Ensure(), який
// не біжить, поки гравець стоїть у паузі, -- і пуші летіли повз. Екземпляр
// лишаємо: так робить ваніль, і час життя підписки видно по полю.
// Активна («ведена») мітка -- вибір КЛІЄНТА і лише його: сервер про неї
// не знає, перезайшов -- вибір скинувся. Одна на весь клієнт: вести дві
// цілі одночасно однаково нема чим.
class OZ_PdaTrack
{
    static string Id    = "";
    static string Name  = "";
    // НЕ «Pos»: рушій вважає Pos ім'ям типу, і Enforce відмовляє
    // змінній з таким ім'ям у статиці.
    static string Point = "";

    static string FmtDist(float d)
    {
        if (d < 1000)
            return Math.Round(d).ToString() + " m";

        float km = Math.Round(d / 100) / 10.0;
        return km.ToString() + " km";
    }

    static string DistanceTo(vector from)
    {
        if (Id == "")
            return "";

        vector to = Point.ToVector();
        return FmtDist(vector.Distance(Vector(from[0], 0, from[2]), Vector(to[0], 0, to[2])));
    }
}

// Активний маршрут: нитка з копій міток, яку клієнт веде точку за
// точкою. Дані маршруту живуть у пристрої (сервер); АКТИВАЦІЯ -- стан
// клієнта, як і ведення одиночної мітки: перезайшов -- веди заново.
class OZ_PdaRoute
{
    // БЕЗ ініціалізації на місці: статичний new у декларації НЕ
    // виконується (зміряно живим клієнтом 2026-08-30 -- null валив
    // PaintMini посеред кадру). Лінива побудова і null-guard всюди.
    static ref array<ref OZ_MapMarker> Points;
    static int  At = 0;
    static bool Active = false;

    static void Start(array<ref OZ_MapMarker> pts)
    {
        if (!Points)
            Points = new array<ref OZ_MapMarker>();

        Points.Clear();
        if (pts)
        {
            for (int i = 0; i < pts.Count(); i++)
                Points.Insert(pts[i]);
        }
        At = 0;
        Active = Points.Count() > 0;
    }

    static void Stop()
    {
        if (Points)
            Points.Clear();
        At = 0;
        Active = false;
    }

    // Точку пройдено -- вручну кнопкою чи ногами (близькість міряє HUD).
    static void Advance()
    {
        At++;
        if (!Points || At >= Points.Count())
            Stop();
    }

    static OZ_MapMarker Current()
    {
        if (!Active || !Points || At >= Points.Count())
            return null;
        return Points[At];
    }

    static int CountSafe()
    {
        if (!Points)
            return 0;
        return Points.Count();
    }
}

class OZ_PdaHudEars
{
    void OnPush(string pageId, string op, bool ok, string json, string error)
    {
        OZ_PdaHud.OnPush(pageId, op, ok, json, error);
    }
}

// Одна панель HUD у реєстрі: віджет, типове місце і людський підпис.
class OZ_HudPane
{
    string Id;
    Widget W;
    float  DefX;
    float  DefY;
    string Label;
}

class OZ_PdaHud
{
    private static Widget s_Root;
    private static Widget s_Toast;
    private static TextWidget s_MiniTrack;
    // Кеш маячків для мінікарти: сервер ПУШИТЬ їх сам власникам працюючих
    // антен -- HUD лише слухає (нуль запитів з клієнта).
    private static ref array<ref OZ_MapBeacon> s_Beacons;
    // Номер останньої посилки маячків. Мінікарта перемальовується, коли він
    // змінився, -- це дешевше за звірку самого списку.
    private static int s_BeaconSeq = 0;
    // Клієнтські ручки з Tuning.json приїздять у пуші маячків.
    private static int s_AdvanceM = 30;
    private static float s_ToastHoldMs = 8000;
    private static Widget s_Mini;
    private static TextWidget s_ToastWho;
    private static TextWidget s_ToastText;
    private static MapWidget s_MiniMap;
    private static float s_Acc = 0;
    private static bool s_Hooked = false;
    private static ref OZ_PdaHudEars s_Ears;
    private static ref array<ref OZ_HudPane> s_Panes = new array<ref OZ_HudPane>();

    // Тост тримається стільки, скільки людина читає один рядок.
    private static float s_ToastUntil = 0;
    private static bool s_ToastShown = false;

    // Раз на півсекунди. Заряд падає повільно, а от «дістав/сховав» гравець
    // помічає одразу, і чекати цілу секунду тут відчутно.
    private static const float TICK = 0.5;

    // Масштаб мінімапи: близько, «де я і що поруч». Ванільна мапа відкриває
    // себе на ~0.33; чверть від того дає квартал, а не область.
    private static const float MINI_SCALE = 0.08;

    private static const string ICON_SELF = "\\DZ\\gear\\navigation\\data\\map_tshelter_ca.paa";
    private static const string ICON_TRACK = "\\DZ\\gear\\navigation\\data\\map_tsign_ca.paa";
    private static const string ICON_BEACON = "\\DZ\\gear\\navigation\\data\\map_transmitter_ca.paa";

    static void Update(float timeslice)
    {
#ifdef NO_GUI
        return;
#endif

        // Вуха озброюються ПЕРШИМ же тактом, а не першою видимістю: пуш,
        // що прийшов, поки гравець стояв у паузі, теж мусить подзвонити --
        // хай і тоді, коли HUD знову стане видно.
        if (!s_Hooked)
        {
            s_Hooked = true;
            s_Ears = new OZ_PdaHudEars();
            OZ_ClientState.ResponseWatch().Insert(s_Ears.OnPush);
        }

        s_Acc += timeslice;
        if (s_Acc < TICK)
            return;
        s_Acc = 0;

        // Гасне разом із ванільним інтерфейсом, а не світиться поверх нього.
        if (!Visible())
        {
            if (s_Root)
                s_Root.Show(false);
            return;
        }

        // ОВЕРЛЕЇ -- ЛИШЕ З НАДІТОГО ПРИЛАДУ (ТЗ-5 R-B1.1).
        //
        // Не Device(): той відкочується на руки, і саме цей відкат ТЗ-5
        // знімає. Трофей у руках екран відкриває, а худ -- ні.
        OZ_PDA_Base pda = Worn();

        if (!pda)
        {
            if (s_Root)
                s_Root.Show(false);
            return;
        }

        // Замкнений пристрій для носія НІМИЙ: ні смужки, ні тостів, ні
        // мінікарти. Хто не ввів код -- той нічого й не бачить; обидва біти
        // синхронні, RPC не потрібен.
        if (pda.OZ_LockedForViewer())
        {
            if (s_Root)
                s_Root.Show(false);
            return;
        }

        Ensure();
        if (!s_Root)
            return;


        s_Root.Show(true);

        PaintToast(pda);
        PaintMini(pda);

        // Маячки HUD більше не просить: сервер сам пушить їх власникам
        // працюючих антен (OZ_PdaHandlerMap.PushBeacons) -- нуль запитів
        // із боку клієнта.
    }

    private static void PaintToast(OZ_PDA_Base pda)
    {
        if (!s_Toast)
            return;

        // Вимкнений пристрій не «пікає»: дзвінок -- робота пристрою.
        bool show = pda.OZ_IsOn() && GetGame().GetTime() < s_ToastUntil;
        if (show != s_ToastShown)
        {
            s_ToastShown = show;
            OZ_Log.Dbg("hud: toast show=" + show.ToString() + " now=" + GetGame().GetTime().ToString() + " until=" + s_ToastUntil.ToString());
        }
        s_Toast.Show(show);
    }

    // Що саме намальовано на мінікарті ЗАРАЗ: місце гравця, номер посилки
    // маячків і стан ведення. Поки жодне з цього не змінилось, перемальовувати
    // нема чого.
    private static vector s_MiniAt = "0 0 0";
    private static int    s_MiniSeq = -1;
    private static string s_MiniWhat = "";
    private static bool   s_MiniDrawn = false;

    private static void PaintMini(OZ_PDA_Base pda)
    {
        if (!s_Mini || !s_MiniMap)
            return;

        if (!pda.OZ_IsOn())
        {
            s_Mini.Show(false);
            s_MiniDrawn = false;
            return;
        }

        PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
        if (!p)
        {
            s_Mini.Show(false);
            s_MiniDrawn = false;
            return;
        }

        s_Mini.Show(true);

        vector at = p.GetPosition();

        // МЕТР -- ЦЕ ЦІНА ОДНОГО ПІКСЕЛЯ І ОДНОЇ ЦИФРИ.
        //
        // Перемальовка стирала й наново клала КОЖНУ мітку двічі на секунду --
        // разом зі скиданням масштабу й позиції, -- навіть коли гравець стоїть
        // на місці. Нижче метра на цьому масштабі не рухається жодна мітка, а
        // рядок відстані округлений до метра й теж не змінюється.
        string what = OZ_PdaTrack.Id + "|" + OZ_PdaRoute.At.ToString() + "|" + OZ_PdaRoute.Active.ToString();
        bool moved = vector.Distance(at, s_MiniAt) >= 1.0;
        if (s_MiniDrawn && !moved && s_MiniSeq == s_BeaconSeq && s_MiniWhat == what)
            return;

        s_MiniAt    = at;
        s_MiniSeq   = s_BeaconSeq;
        s_MiniWhat  = what;
        s_MiniDrawn = true;

        s_MiniMap.SetMapPos(at);
        s_MiniMap.ClearUserMarks();
        s_MiniMap.AddUserMark(at, "", OZ_PdaConst.MARK_SELF, ICON_SELF);

        // Чужі маячки -- ті самі, що на великій карті: антена вже все
        // відфільтрувала на сервері.
        if (s_Beacons)
        {
            for (int bb = 0; bb < s_Beacons.Count(); bb++)
                s_MiniMap.AddUserMark(s_Beacons[bb].Pos.ToVector(), "", OZ_PdaConst.MARK_BEACON, ICON_BEACON);
        }

        // МАРШРУТ б'є одиночне ведення: точка нитки на мінікарті, рядок
        // «назва (k/n) відстань» під нею, і авто-прохід -- підійшов на
        // 30 метрів, нитка сама перейшла до наступної точки.
        OZ_MapMarker rc = OZ_PdaRoute.Current();
        if (rc)
        {
            vector rp = rc.Pos.ToVector();
            float rd = vector.Distance(Vector(at[0], 0, at[2]), Vector(rp[0], 0, rp[2]));
            if (rd < s_AdvanceM)
            {
                OZ_PdaRoute.Advance();
                s_MiniWhat = "";   // нитка пішла далі -- наступний такт малює
                rc = OZ_PdaRoute.Current();
                if (rc)
                {
                    rp = rc.Pos.ToVector();
                    rd = vector.Distance(Vector(at[0], 0, at[2]), Vector(rp[0], 0, rp[2]));
                }
            }

            if (rc)
            {
                s_MiniMap.AddUserMark(rp, "", OZ_PdaConst.MARK_ROUTE, ICON_TRACK);
                if (s_MiniTrack)
                {
                    s_MiniTrack.Show(true);
                    s_MiniTrack.SetText(rc.Name + " (" + (OZ_PdaRoute.At + 1).ToString() + "/" + OZ_PdaRoute.CountSafe().ToString() + ")  " + OZ_PdaTrack.FmtDist(rd));
                }
                return;
            }
        }

        // Ведена мітка: точка на мінікарті й рядок «назва + відстань» під
        // нею. Відстань жива -- рахується щокадру з позиції гравця, це
        // одне віднімання і воно нічого не коштує.
        if (OZ_PdaTrack.Id != "")
            s_MiniMap.AddUserMark(OZ_PdaTrack.Point.ToVector(), "", OZ_PdaConst.MARK_BEACON, ICON_TRACK);

        if (s_MiniTrack)
        {
            if (OZ_PdaTrack.Id != "")
            {
                s_MiniTrack.Show(true);
                string tn = OZ_PdaTrack.Name;
                if (tn == "")
                    tn = Widget.TranslateString("#STR_OZ_MAP_UNNAMED");
                s_MiniTrack.SetText(tn + "  " + OZ_PdaTrack.DistanceTo(at));
            }
            else
                s_MiniTrack.Show(false);
        }

    }

    // Пуш із сервера: рядок чату приїхав, байдуже, відкрите меню чи ні.
    // Свої не дзвонять; відкрите меню показує рядок краще за тост, але
    // Visible() і так гасить увесь HUD, поки будь-яке меню відкрите.
    static void OnPush(string pageId, string op, bool ok, string json, string error)
    {
        // Свіжий пост новин дзвонить усім: заголовок у тост, текст -- на
        // сторінці, коли відкриють.
        if (pageId == OZ_PdaConst.PAGE_NEWS && op == "push" && ok)
        {
            OZ_NewsPush np = new OZ_NewsPush();
            string nerr;
            if (!JsonFileLoader<OZ_NewsPush>.LoadData(json, np, nerr) || !np)
                return;

            // ДЗВОНИТЬ ЛИШЕ НОВИЙ ДОПИС. Конверт їде на будь-яку зміну
            // стрічки -- сторінка скидає по ньому свій кеш, -- але
            // виправлений чи стертий пост тостом не є.
            if (!np.Fresh)
                return;

            // Знімаємо обидва рядки ДО Ensure(): він будує ціле дерево
            // віджетів, а конверт виділив серіалізатор.
            string nwho   = np.Who;
            string ntitle = np.Title;

            Ensure();
            if (s_ToastWho)
            {
                s_ToastWho.SetText(Widget.TranslateString("#STR_OZ_TOAST_NEWS") + "  " + nwho);
                s_ToastWho.SetColor(OZ_Palette.ACCENT);
            }
            if (s_ToastText)
                s_ToastText.SetText(ntitle);
            s_ToastUntil = GetGame().GetTime() + s_ToastHoldMs;
            return;
        }

        // Пуш маячків від сервера: список і дві клієнтські ручки тюнінгу.
        if (pageId == OZ_PdaConst.PAGE_MAP && op == "beacons" && ok)
        {
            OZ_BeaconPush bp = new OZ_BeaconPush();
            string berr;
            if (JsonFileLoader<OZ_BeaconPush>.LoadData(json, bp, berr) && bp)
            {
                if (!s_Beacons)
                    s_Beacons = new array<ref OZ_MapBeacon>();
                s_Beacons.Clear();
                if (bp.Beacons)
                {
                    // Копія: статик живе всю місію, а міні-карта читає його
                    // двічі на секунду.
                    for (int pb = 0; pb < bp.Beacons.Count(); pb++)
                    {
                        if (bp.Beacons[pb])
                            s_Beacons.Insert(bp.Beacons[pb].Copy());
                    }
                }
                s_BeaconSeq++;
                if (bp.AdvanceM > 0)
                    s_AdvanceM = bp.AdvanceM;
                if (bp.ToastS > 0)
                    s_ToastHoldMs = bp.ToastS * 1000;
            }
            return;
        }

        // Стан карти: забираємо маячки для мінікарти, коли сторінка карти
        // відкрита і сама спитала. Відповідь одна на всіх.
        if (pageId == OZ_PdaConst.PAGE_MAP && op == "state" && ok)
        {
            OZ_MapState ms = new OZ_MapState();
            string merr;
            if (JsonFileLoader<OZ_MapState>.LoadData(json, ms, merr) && ms)
            {
                if (!s_Beacons)
                    s_Beacons = new array<ref OZ_MapBeacon>();
                s_Beacons.Clear();
                if (ms.Beacons)
                {
                    for (int bi = 0; bi < ms.Beacons.Count(); bi++)
                    {
                        if (ms.Beacons[bi])
                            s_Beacons.Insert(ms.Beacons[bi].Copy());
                    }
                }
                s_BeaconSeq++;
            }
            return;
        }

        if (pageId != OZ_PdaConst.PAGE_CHAT || op != "line" || !ok)
            return;

        OZ_Log.Dbg("hud: chat line push heard");

        OZ_ChatPush p = new OZ_ChatPush();
        string err;
        if (!JsonFileLoader<OZ_ChatPush>.LoadData(json, p, err) || !p)
            return;

        if (p.Mine)
            return;

        // Усе з конверта -- ДО Ensure(): він будує дерево віджетів, а далі
        // йдуть склейки й переклади, і кожне з цього -- виділення.
        string pkind  = p.Kind;
        string ptitle = p.Title;
        string pwho   = p.Who;
        string ptext  = p.Text;
        int    pcolor = p.WhoColor;

        Ensure();
        if (s_ToastWho)
        {
            // ЗВІДКИ прийшло -- прямо в заголовку: група на ім'я, пейджер,
            // Зона чи особисте. Хто пише -- фарбується фракцією.
            string chan = "#STR_OZ_TOAST_DM";
            if (pkind == "group")
                chan = ptitle;
            else if (pkind == "npc")
                chan = "#STR_OZ_CHAT_PAGER";
            else if (pkind == "zone")
                chan = "#STR_OZ_CHAT_ZONE";

            s_ToastWho.SetText(pwho + "   [" + Widget.TranslateString(chan) + "]");
            if (pcolor != 0)
                s_ToastWho.SetColor(pcolor);
            else
                s_ToastWho.SetColor(OZ_Palette.ACCENT);
        }
        if (s_ToastText)
            s_ToastText.SetText(ptext);
        s_ToastUntil = GetGame().GetTime() + s_ToastHoldMs;
        OZ_Log.Dbg("hud: toast armed until=" + s_ToastUntil.ToString() + " who=" + p.Who);
    }

    // Той самий пристрій, про який говорить сервер, коли йдеться про
    // СТОРІНКИ Й ВОРОТА: слот носіння, потім руки. Інакше меню й сервер
    // говорили б про різні речі, і саме це тут і було: код брав надітий,
    // коментар над ним обіцяв «руки, потім слот», а серверний
    // OZ_PdaLookup.HeldByPlayer робив саме те, що обіцяв коментар.
    //
    // ОВЕРЛЕЇ СЮДИ НЕ ХОДЯТЬ: у них своє правило, Worn() нижче (ТЗ-5
    // R-B1.5 -- правила два, і різниця між ними навмисна).
    //
    // Публічний: меню теж малює заряд і замок з локальної сутності,
    // без жодного запиту -- пристрій той самий.
    static OZ_PDA_Base Device()
    {
        // ПРАВИЛО ЖИВЕ В OZ_PdaLookup, а не тут.
        //
        // Тут стояла ДРУГА копія «надітий, потім руки» -- слово в слово те
        // саме, що робить HeldByPlayer, -- і саме через дві копії худ і
        // сервер уже раз розійшлися: комент над однією обіцяв «руки, потім
        // слот», а робила вона навпаки. Тепер копія одна.
        return OZ_PdaLookup.HeldByPlayer(PlayerBase.Cast(GetGame().GetPlayer()));
    }

    // Прилад, з якого мальовані оверлеї: ТІЛЬКИ НАДІТИЙ (ТЗ-5 R-B1.1,
    // R-B1.2). Друга половина того самого правила -- у
    // OZ_PdaLookup.WornBy, і копії тут теж немає.
    private static OZ_PDA_Base Worn()
    {
        return OZ_PdaLookup.WornBy(PlayerBase.Cast(GetGame().GetPlayer()));
    }

    // Два числа з пакета синхронізації ядра (D87). Посилка маячків їх теж
    // везе -- і лишається джерелом для того, хто її отримує; але цей шлях
    // доходить до КОЖНОГО, а не лише до власника антени.
    static void ApplySync()
    {
        int adv = OZ_ClientState.Extra(OZ_PdaConst.SYNC_ROUTE_M, "").ToInt();
        if (adv > 0)
            s_AdvanceM = adv;

        int toast = OZ_ClientState.Extra(OZ_PdaConst.SYNC_TOAST_S, "").ToInt();
        if (toast > 0)
            s_ToastHoldMs = toast * 1000;

        string line = "hud: sync gave toast=" + toast.ToString() + "s";
        line += " advance=" + adv.ToString() + "m";
        OZ_Log.Dbg(line);
    }

    private static void Ensure()
    {
        if (s_Root)
            return;

        s_Root = GetGame().GetWorkspace().CreateWidgets("OpenZone_PDA/gui/layouts/oz_pda_hud.layout");
        if (!s_Root)
        {
            OZ_Log.Error("pda hud layout failed to load");
            return;
        }

        s_Toast     = s_Root.FindAnyWidget("ToastPane");
        s_ToastWho  = TextWidget.Cast(s_Root.FindAnyWidget("ToastWho"));
        s_ToastText = TextWidget.Cast(s_Root.FindAnyWidget("ToastText"));
        s_Mini      = s_Root.FindAnyWidget("MiniPane");
        s_MiniMap   = MapWidget.Cast(s_Root.FindAnyWidget("MiniMap"));
        s_MiniTrack = TextWidget.Cast(s_Root.FindAnyWidget("MiniTrack"));

        // Мінікарта малюється в СВОЇЙ системі координат і не рухається:
        // масштаб ставимо тут один раз, а не на кожну перемальовку.
        if (s_MiniMap)
            s_MiniMap.SetScale(MINI_SCALE);

        // Свої панелі йдуть через той самий реєстр, що й чужі: власна їжа.
        Adopt("toast", s_Toast, 0.655, 0.825, "#STR_OZ_HUD_PANE_TOAST");
        Adopt("mini",  s_Mini,  0.845, 0.6,  "#STR_OZ_HUD_PANE_MINI");
    }

    // ------------------------------------------------------------ реєстр
    //
    // ПУБЛІЧНИЙ ВХІД ДЛЯ МОДУЛІВ: чужий мод створює свій віджет на робочій
    // області сам і віддає його сюди з унікальним id -- отримує збережену
    // позицію гравця, місце в редакторі розкладки і типове місце на випадок,
    // коли гравець ще нічого не рухав. Ховати/показувати віджет мод
    // продовжує сам: реєстр володіє ПОЗИЦІЄЮ, не видимістю.
    static void Adopt(string id, Widget w, float defX, float defY, string label)
    {
        if (!w || id == "")
            return;

        OZ_HudPane pane;
        for (int i = 0; i < s_Panes.Count(); i++)
        {
            if (s_Panes[i].Id == id)
            {
                pane = s_Panes[i];
                break;
            }
        }

        if (!pane)
        {
            pane = new OZ_HudPane();
            s_Panes.Insert(pane);
        }

        pane.Id    = id;
        pane.W     = w;
        pane.DefX  = defX;
        pane.DefY  = defY;
        pane.Label = label;

        float x = defX;
        float y = defY;
        OZ_PdaHudLayout.Get(id, x, y);
        w.SetPos(x, y);
    }

    static array<ref OZ_HudPane> Panes()
    {
        return s_Panes;
    }

    // Редактор розкладки мусить бачити панелі навіть якщо HUD ще ЖОДНОГО
    // разу не був видимим (гравець відкрив КПК одразу, Ensure не бігав, і
    // реєстр порожній -- редактор без жодної рамки). Створюємо віджети
    // примусово; видимістю далі керує звичайний Update.
    static void EnsurePanes()
    {
        Ensure();
    }

    // Перечитати збережені позиції для всіх панелей -- редактор кличе це
    // після APPLY, щоб живий HUD став туди, куди його поклали.
    static void Reapply()
    {
        for (int i = 0; i < s_Panes.Count(); i++)
        {
            OZ_HudPane pane = s_Panes[i];
            if (!pane.W)
                continue;

            float x = pane.DefX;
            float y = pane.DefY;
            OZ_PdaHudLayout.Get(pane.Id, x, y);
            pane.W.SetPos(x, y);
        }
    }

    // Чи має HUD бути видимим ЗАРАЗ. Одна функція на всі причини гасіння
    // -- інакше кожне нове місце гасило б по-своєму, і одне з них рано чи
    // пізно розійшлося б з рештою.
    private static bool Visible()
    {
        PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
        if (!p || !p.IsAlive())
            return false;

        UIManager ui = GetGame().GetUIManager();
        if (!ui)
            return false;

        // Наш власний КПК відкритий -- екран каже все те саме, і краще.
        if (ui.FindMenu(OZ_PdaConst.MENU_PDA))
            return false;

        // Будь-яке інше меню поверх світу: пауза, налаштування, чуже вікно.
        if (ui.GetMenu())
            return false;

        // Прапорці ванільного інтерфейсу. IngameHud.Cast -- саме так до них
        // ходить і сама ваниль (gesturesmenu.c:228, continuousactionprogress.c:60).
        IngameHud hud = IngameHud.Cast(GetGame().GetMission().GetHud());
        if (hud)
        {
            IngameHudVisibility vis = hud.GetHudVisibility();
            if (vis)
            {
                // По одній перевірці на рядок, і це не стиль. Умова `if`
                // у Enforce мусить уміщатися в ОДИН рядок: перенесення дає
                // "Expected ')', not a '||'" незалежно від того, стоїть
                // оператор наприкінці рядка чи на початку наступного.
                // Перевірено на стенді 2026-08-26, обидва варіанти.
                if (vis.IsContextFlagActive(EHudContextFlags.HUD_HIDE))
                    return false;
                if (vis.IsContextFlagActive(EHudContextFlags.HUD_DISABLE))
                    return false;
                if (vis.IsContextFlagActive(EHudContextFlags.INVENTORY_OPEN))
                    return false;
                if (vis.IsContextFlagActive(EHudContextFlags.MENU_OPEN))
                    return false;
                if (vis.IsContextFlagActive(EHudContextFlags.UNCONSCIOUS))
                    return false;
            }
        }

        return true;
    }

    // Знімає корінь із робочої області.
    //
    // Ensure() створює віджет ПРЯМО на робочій області, а не всередині чийогось
    // дерева, тож ніхто, крім нас, його не прибере. Досі мод не мав
    // OnMissionFinish узагалі -- отже кожен перезапуск місії лишав по одній
    // смужці назавжди.
    static void Teardown()
    {
        // Дзеркало підписки: інвокер ядра статичний і переживе місію, а
        // вуха тримають шлях до мертвих віджетів. Знімаємо і дозволяємо
        // наступній місії підписатись заново.
        if (s_Ears)
            OZ_ClientState.ResponseWatch().Remove(s_Ears.OnPush);
        s_Hooked = false;

        if (s_Root)
        {
            s_Root.Unlink();
            s_Root = null;
        }

        s_Toast     = null;
        s_ToastWho  = null;
        s_ToastText = null;
        s_Mini      = null;
        s_MiniMap   = null;
        s_MiniTrack = null;
        s_Ears      = null;
        s_Panes.Clear();
        s_Acc       = 0;
        s_MiniDrawn = false;
        s_MiniWhat  = "";
        s_MiniSeq   = -1;
        s_BeaconSeq = 0;
        s_ToastUntil = 0;
        s_ToastShown = false;

        // ДАНІ ТЕЖ, а не самі віджети.
        //
        // Прибирались лише посилання на віджети, а все, що вони малювали,
        // лишалось у статиках -- вони переживають місію. Наступний сервер
        // діставав у спадок чужі маячки, чужий активний маршрут, чужу
        // ведену мітку й числа тюнінгу того сервера. Оверлей малював їх до
        // першого пуша нового сервера, а сервер БЕЗ КПК не пришле його
        // ніколи: маячки минулої Зони світились би на карті вічно.
        if (s_Beacons)
            s_Beacons.Clear();

        OZ_PdaRoute.Stop();

        OZ_PdaTrack.Id    = "";
        OZ_PdaTrack.Name  = "";
        OZ_PdaTrack.Point = "";

        // Тюнінг -- назад на вбудовані числа: чужі приїдуть із першим пушем.
        s_AdvanceM    = 30;
        s_ToastHoldMs = 8000;
    }
}
