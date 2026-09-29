// Сторінка «Зв'язок»: особисті й групові розмови.
//
// ДЖЕРЕЛО ПРАВДИ -- DISCORD. Сервер розмов не тримає взагалі: ні файлів, ні
// пам'яті. Він перекладає прохання гравця мостові й віддає назад те, що
// відповів Discord.
//
// Найважливіше наслідок: власне повідомлення НЕ з'являється в розмові
// одразу. Воно йде в Discord, і в розмову його вносить ЕХО, яке приїжджає
// довгим опитом за кілька мілісекунд. Це не затримка, яку треба обійти
// оптимістичним показом -- це і є визначення «Discord є правдою». Показати
// рядок раніше означало б показати те, чого в розмові ще немає, а якщо
// Discord його не прийме -- то й не буде.
//
// ХТО КОМУ МОЖЕ ПИСАТИ вирішує СЕРВЕР, а не міст. Особисту розмову можна
// почати лише з КОНТАКТОМ -- тим, з ким уже потиснули руки (див.
// OZ_PdaContacts). Це не обмеження заради обмеження: без нього кожен міг би
// написати кожному, знаючи лише ім'я. Розмови належать Discord, але право
// їх заводити -- ігровій механіці, і воно лишається тут.
//
// ВІДПОВІДІ ВІДКЛАДЕНІ. RestContext асинхронний, тож Handle() не має чого
// повернути: він каже OZ_Const.DEFER і відповідає сам, коли міст озветься.

// ------------------------------------------------------- листи до моста

class OZ_ChatAskMine
{
    string Until;
    string Uid;
}

class OZ_ChatAskOpen
{
    string Until;
    string Uid;
    string Id;
    int    Limit;
}

class OZ_ChatAskOlder
{
    string Until;
    string Uid;
    string Id;
    string Before;
    int    Limit;
}

class OZ_ChatAskSend
{
    string Uid;
    string Name;
    string Id;
    string Text;
    bool   Anon;
}

class OZ_ChatAskStart
{
    string Uid;
    string Name;
    string OtherUid;
    string OtherName;

    // КЛЮЧІ ПЕРСОНАЖІВ, з яких міст робить id розмови. Steam64 для цього не
    // годиться: після пермадесу той самий акаунт -- уже інша людина, а id,
    // зроблений із пари Steam64, привів би її в чужу розмову з небіжчиком,
    // разом з усією його перепискою.
    //
    // Доставка й склад лишаються за Steam64 -- пише все-таки акаунт.
    string MyKey;
    string OtherKey;
}

class OZ_ChatAskGroup
{
    string Uid;
    string Title;
    string Desc;
    // Скільки груп цей прилад дозволяє заснувати (Limits.GroupChats профілю,
    // ТЗ-4 R-F1.6). Рахує міст -- групи живуть у нього; нуль -- без межі.
    int    Max;
}

// Правка існуючої групи: назва й опис. Порожнє поле -- «не чіпати» вирішує
// міст, який єдиний знає поточні значення.
class OZ_ChatAskGroupEdit
{
    string Uid;
    string Id;
    string Title;
    string Desc;
}

// Видалення групи: {Uid, Id}. Раніше тут їздив лист записок -- записки
// відв'язано від моста, тож у розмов тепер свій конверт.
class OZ_ChatAskGroupDel
{
    string Uid;
    string Id;
}

// Пара для замороження/розмороження особистої розмови: контакт розірвано
// (чи відновлено) -- тред у Discord замикається (відмикається), читання
// лишається. Груп це не стосується.
class OZ_ChatAskPair
{
    string A;
    string B;
}

// Фарбування імен фракційним кольором -- на СЕРВЕРІ. Таблиця кольорів
// живе в OZ_Factions, і роздавати її клієнтові не треба: міст шле
// Steam64 автора (AUid), сервер міняє його на готовий ARGB і стирає.
class OZ_ChatColors
{
    // Фарбує рядки розмови НА МІСЦІ (розмова вже скопійована викликачем --
    // див. OZ_ChatIds.Rewrite) і стирає Steam64 авторів.
    //
    // КОЛІР КОЖНОГО АВТОРА -- РАЗ. Історія на двадцять рядків -- це зазвичай
    // двоє-троє авторів, а колір питався на кожен рядок: угруповання автора
    // через службу ідентичності, і для офлайнового автора це було читання
    // його файлу.
    static void PaintView(OZ_ChatView v)
    {
        if (!v || !v.Lines)
            return;

        map<string, int> memo = new map<string, int>();
        for (int i = 0; i < v.Lines.Count(); i++)
        {
            OZ_ChatLine l = v.Lines[i];
            if (!l)
                continue;

            int col;
            if (!memo.Find(l.AUid, col))
            {
                col = ColorFor(l.AUid);
                memo.Set(l.AUid, col);
            }
            l.WhoColor = col;
            l.AUid = "";
        }
    }

    // ОДНЕ ПРАВИЛО НА ОБИДВА ШЛЯХИ -- історію й живий рядок.
    //
    // УГРУПОВАННЯ, а не базова (ТЗ-1 §5): базова є в кожного, тож колір за
    // нею пофарбував би весь чат в один відтінок і не сказав нічого. Нуль
    // означає «без кольору», і саме його чекає клієнт.
    //
    // Друга копія цього правила жила в OZ_ChatSink.Deliver, і розійтися їм
    // не було де лише тому, що обидві написали в один день.
    static int ColorFor(string auid)
    {
        if (auid == "")
            return 0;

        string fac = OZ_Identity.Get().OrgOf(auid);
        if (fac == "")
            return 0;

        return OZ_Identity.Get().FactionColor(fac, 255);
    }
}

class OZ_PairFreeze
{
    // reply = null -- «відповідь нікому не потрібна»: так ідуть відмикання
    // (pair_thaw) після повторного рукостискання -- якщо міст спить,
    // розмова лишиться в попередньому стані до наступної нагоди.
    //
    // ЗАМОРОЖЕННЯ ЙДЕ ІНАКШЕ (ТЗ-5 R-F3.1): контакт не викреслюється, доки
    // міст не підтвердив, тож той, хто його шле, дає сюди свій
    // OZ_BridgeReply і робить решту вже в ньому.
    //
    // true -- лист пішов; false -- його не вдалося навіть скласти, і
    // викликач мусить відповісти гравцеві сам.
    static bool Send(string route, string a, string b, OZ_BridgeReply reply = null)
    {
        OZ_ChatAskPair pr = new OZ_ChatAskPair();
        pr.A = a;
        pr.B = b;

        string letter;
        string err;
        if (!JsonFileLoader<OZ_ChatAskPair>.MakeData(pr, letter, err, false))
            return false;

        OZ_BridgeClient.Call(route, letter, reply);
        return true;
    }
}

class OZ_ChatAskInvite
{
    // Стеля складу групи з Tuning.json; склад знає лише міст, тому межа
    // їде разом із запрошенням. 0 -- без межі.
    int Max = 0;
    // Скільки секунд живе запрошення (ТЗ-4 R-D3.3); нуль -- безстроково.
    int TtlS = 0;
    string Uid;
    string Id;
    string OtherUid;
}

// Відмова моста. Код -- машинний («no_chat»), а не готове речення: мова
// гравця відома лише клієнтові, і рядки для неї лежать у stringtable.
class OZ_ChatFail
{
    string Error;

    static string KeyOf(string code)
    {
        if (code == "no_chat")
            return "STR_OZ_ERR_NO_CHAT";
        if (code == "not_group")
            return "STR_OZ_ERR_NOT_GROUP";
        if (code == "already_in")
            return "STR_OZ_ERR_GROUP_ALREADY_IN";
        if (code == "discord_down")
            return "STR_OZ_ERR_NO_BRIDGE";
        if (code == "read_only")
            return "STR_OZ_ERR_READ_ONLY";
        if (code == "groups_full")
            return "STR_OZ_ERR_GROUPS_FULL";
        if (code == "not_owner")
            return "STR_OZ_ERR_NOT_OWNER";
        if (code == "group_full")
            return "STR_OZ_ERR_GROUP_FULL";
        return "STR_OZ_ERR_PDA_INTERNAL";
    }
}

// Рядок, який приїхав опитом.
//
// Uid -- рахунок-одержувач, і клієнтові він НЕ їде: сервер читає його й
// стирає. «Свій же Steam64 він і так знає» було правдою лише для свого
// приладу; тримач чужого живого термінала отримував Steam64 власника.
// Так само Id -- конверт їде з токеном розмови, а не з ключем моста
// (OZ_ChatIds): ключ особистої розмови складений зі Steam64 обох.
class OZ_ChatPush
{
    string AUid = "";
    int    WhoColor = 0;
    // Куди прийшов рядок: рід розмови і назва (для груп) -- тост показує
    // канал у заголовку. "invite" -- запрошення до групи (міст 0.8.2).
    string Kind = "";
    string Title = "";
    string Uid;
    string Id;
    string At;
    string Who;
    string Text;
    bool   Mine;
    // Анонімний рядок Зони: ім'я малює клієнт своєю мовою.
    bool   Anon = false;
    // Міст укоротив текст, щоб конверт пройшов 1023-байтову стелю розбору;
    // повний рядок віддає open.
    bool   Clipped = false;
    // Чи цей рядок для НАДІТОГО приладу, тобто для худа (ТЗ-5 R-B1.1).
    // Екран чату бере будь-який; тост -- лише цей.
    bool   ToHud = false;
}

// Хто отримує живий рядок і для якого приладу: надітого (ToHud) чи того, що в
// руках (екран).
class OZ_ChatHolder
{
    PlayerIdentity Id;
    bool ToHud;
}

// ТОКЕНИ РОЗМОВ замість ключів моста.
//
// Ключ особистої розмови в мості -- "d:<steam64>#<покоління>:<steam64>#<покоління>",
// групи -- "g:<steam64 засновника>:...", запрошення несе ключ групи. Сервер
// пересилав тіла моста як є, і будь-який клієнт читав Steam64 кожного
// співрозмовника, навіть офлайнового, і засновника групи -- з одного
// запрошення. Правило серії «клієнт чужого Steam64 не бачить» (OZ_Names)
// ламалось саме тут.
//
// Токен -- номер за порядком появи, і більше в ньому нічого немає. Мапа
// живе один запуск сервера: після рестарту клієнт однаково перепитує
// перелік розмов і отримує нові токени.
class OZ_ChatIds
{
    private static ref map<string, string> s_ByToken;
    private static ref map<string, string> s_ById;
    private static int s_Next = 0;

    static string Out(string id)
    {
        if (id == "")
            return "";

        if (!s_ById)
        {
            s_ById    = new map<string, string>();
            s_ByToken = new map<string, string>();
        }

        string t;
        if (s_ById.Find(id, t))
            return t;

        s_Next++;
        t = "c" + s_Next.ToString();
        s_ById.Set(id, t);
        s_ByToken.Set(t, id);
        return t;
    }

    // Ключ моста за токеном; порожньо -- такого токена сервер не видавав
    // (підробка або рестарт).
    static string In(string token)
    {
        if (token == "" || !s_ByToken)
            return "";

        string id;
        if (s_ByToken.Find(token, id))
            return id;
        return "";
    }

    // Тіло відповіді моста -> те, що їде клієнтові: ключі стають токенами,
    // Steam64 авторів -- кольорами. Копія -- до перших виділень (шапка
    // OZ_PdaTypes).
    static string Rewrite(string op, string json)
    {
        string err;
        string outJson;

        if (op == "list")
        {
            OZ_ChatList l = new OZ_ChatList();
            if (!JsonFileLoader<OZ_ChatList>.LoadData(json, l, err) || !l)
                return "";
            l = l.Copy();

            int h;
            for (h = 0; h < l.Items.Count(); h++)
                l.Items[h].Id = Out(l.Items[h].Id);
            for (h = 0; h < l.Invites.Count(); h++)
                l.Invites[h].Id = Out(l.Invites[h].Id);

            if (!JsonFileLoader<OZ_ChatList>.MakeData(l, outJson, err, false))
                return "";
            return outJson;
        }

        if (op == "open" || op == "older")
        {
            OZ_ChatView v = new OZ_ChatView();
            if (!JsonFileLoader<OZ_ChatView>.LoadData(json, v, err) || !v)
                return "";
            v = v.Copy();

            v.Id = Out(v.Id);
            OZ_ChatColors.PaintView(v);

            if (!JsonFileLoader<OZ_ChatView>.MakeData(v, outJson, err, false))
                return "";
            return outJson;
        }

        if (op == "start" || op == "group_new")
        {
            OZ_ChatRef r = new OZ_ChatRef();
            if (!JsonFileLoader<OZ_ChatRef>.LoadData(json, r, err) || !r)
                return "";

            r.Id = Out(r.Id);
            if (!JsonFileLoader<OZ_ChatRef>.MakeData(r, outJson, err, false))
                return "";
            return outJson;
        }

        // Решта операцій тіла клієнтові не віддає (див. m_Body).
        return "";
    }
}

// ------------------------------------------------------------ адресат

class OZ_ChatWho
{
    // Особу шукаємо СВІЖОЮ, а не тримаємо посилання з моменту запиту: поки
    // відповідь їхала до Discord і назад, гравець міг вийти, і збережена
    // PlayerIdentity вказувала б у порожнечу.
    //
    // Через ядро, а не власним обходом GetPlayers: OZ_Link.Online робить те
    // саме через OZ_Players.ManOf, тобто без обходу онлайну й без алокації
    // рядка GetPlainId на кожного гравця. Копія тут була старшою за ядерну
    // й пережила її появу.
    static PlayerIdentity Online(string uid)
    {
        return OZ_Link.Online(uid);
    }

    // Ім'я, від якого ГОВОРИТЬ пристрій: власника сесії, а не тримача.
    // Для свого КПК це те саме ім'я; для захопленого -- імперсонація, і
    // вона навмисна: рішення власника 2026-08-28.
    static string NameOf(string accUid, PlayerIdentity sender)
    {
        // Peek: власник сесії приладу може бути де завгодно, зокрема офлайн.
        OZ_PlayerData accPd = OZ_PlayerStore.Peek(accUid);
        if (accPd && accPd.Name != "")
            return accPd.Name;
        return sender.GetName();
    }

    // ЗА ЧИЙ РАХУНОК ГОВОРИТЬ ПРИЛАД ПРЯМО ЗАРАЗ, або порожньо.
    //
    // Жива сесія, увімкнений, ВІДІМКНЕНИЙ. Досі живі рядки йшли будь-кому з
    // приладом, на якому просто була сесія: тримач ЗАМКНЕНОГО чужого КПК
    // отримував особисті рядки власника пушем, а його надітий худ показував
    // їх тостом -- без жодного коду. Ворота сторінок на тому самому приладі
    // відмовляли б кожному запиту; пуш їх обходив. Неініційований прилад
    // теж не говорить ні за кого: його сторінки мовчать (ворота, NOT_INIT).
    // Замок рахуємо ліниво тут же -- так, як ворота й пуш маячків.
    static string SpeaksFor(OZ_PDA_Base dev)
    {
        if (!dev)
            return "";
        if (!dev.OZ_IsOn())
            return "";

        OZ_PdaProfile prof = OZ_PdaProfiles.ForClass(dev.GetType());
        if (prof)
            dev.OZ_EvaluateLock(prof.LockAfterMinutes);
        if (!dev.OZ_IsUnlocked())
            return "";

        string acc = dev.OZ_SessionUid();
        if (acc == "")
            return "";
        if (OZ_PdaCapsule.IsFrozen(dev))
            return "";
        return acc;
    }

    // Хто кого слухає -- ОДИН РАЗ НА КАДР, а не на кожен конверт.
    //
    // Міст шле по конверту на КОЖНОГО одержувача, а рядок ефіру Зони --
    // це конверт на кожного в онлайні; обхід усіх гравців на кожен конверт
    // давав n^2 в одному кадрі. Тепер обхід один, а конверти беруть готове.
    private static ref map<string, ref array<ref OZ_ChatHolder>> s_Memo;
    private static int s_MemoAt = -1;

    private static void Remember(string acc, PlayerIdentity id, bool hud)
    {
        array<ref OZ_ChatHolder> list;
        if (!s_Memo.Find(acc, list))
        {
            list = new array<ref OZ_ChatHolder>();
            s_Memo.Set(acc, list);
        }

        OZ_ChatHolder h = new OZ_ChatHolder();
        h.Id  = id;
        h.ToHud = hud;
        list.Insert(h);
    }

    // ДВА ПРИЛАДИ -- ДВІ ПРАВДИ (ТЗ-5 R-B1): надітий дає худ (тост), той,
    // що в руках, -- екран. Досі рахувався один, і той -- у руках: трофей-
    // капсула в руці глушив рядки власного надітого, а живий чужий КПК у
    // руці вливав у надітий худ тости чужого рахунку.
    private static void Rebuild()
    {
        if (!s_Memo)
            s_Memo = new map<string, ref array<ref OZ_ChatHolder>>();
        s_Memo.Clear();

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase pl = PlayerBase.Cast(players[i]);
            if (!pl)
                continue;

            PlayerIdentity id = pl.GetIdentity();
            if (!id)
                continue;

            // НЕМАЄ ПРИЛАДУ -- НЕМАЄ РЯДКІВ (ТЗ-4 R-G3.1).
            string worn  = SpeaksFor(OZ_PdaLookup.WornBy(pl));
            string hands = SpeaksFor(OZ_PDA_Base.Cast(pl.GetItemInHands()));

            if (worn != "")
                Remember(worn, id, true);
            // Той самий рахунок на обох приладах -- один рядок, і він худу:
            // інакше учасник отримав би той самий рядок двічі.
            if (hands != "" && hands != worn)
                Remember(hands, id, false);
        }
    }

    private static void Fresh()
    {
        int now = GetGame().GetTime();
        if (s_Memo && s_MemoAt == now)
            return;

        Rebuild();
        s_MemoAt = now;
    }

    static void Holders(string uid, array<ref OZ_ChatHolder> outTo)
    {
        Fresh();

        array<ref OZ_ChatHolder> list;
        if (!s_Memo.Find(uid, list))
            return;

        for (int i = 0; i < list.Count(); i++)
            outTo.Insert(list[i]);
    }

    // Усі рахунки, які хтось зараз слухає.
    static void Accounts(array<string> outAcc)
    {
        Fresh();

        for (int i = 0; i < s_Memo.Count(); i++)
            outAcc.Insert(s_Memo.GetKey(i));
    }
}

// Міст дренує розмови лише «онлайн»-акаунтів. Захоплений живий КПК
// говорить за власника й тоді, коли самого власника в Зоні немає, -- його
// акаунт теж мусить бути в списку, інакше тримач не побачить ані чужих
// рядків, ані еха власних відправлень.
//
// Правило те саме, що в доставки (OZ_ChatWho.SpeaksFor): замкнений чи
// вимкнений прилад рахунку не тримає, і опитувати його нема для кого.
class OZ_PdaUidProvider : OZ_BridgeUidProvider
{
    override void Fill(array<string> uids)
    {
        array<string> accs = new array<string>();
        OZ_ChatWho.Accounts(accs);

        // Наявність -- мапою, а не Find по масиву на кожен рахунок.
        map<string, bool> have = new map<string, bool>();
        for (int u = 0; u < uids.Count(); u++)
            have.Set(uids[u], true);

        for (int i = 0; i < accs.Count(); i++)
        {
            if (have.Contains(accs[i]))
                continue;
            have.Set(accs[i], true);
            uids.Insert(accs[i]);
        }
    }
}

// ------------------------------------------------------------ відповіді

class OZ_ChatReply : OZ_BridgeReply
{
    protected string m_Uid;
    protected string m_Op;
    protected bool   m_Body;

    // body=true -- віддати клієнтові тіло відповіді. Форма, якою говорить
    // міст, і форма, якої чекає сторінка, збігаються навмисно -- опис один;
    // сервер лише міняє в ній ключі на токени й Steam64 на кольори
    // (OZ_ChatIds.Rewrite).
    void OZ_ChatReply(string uid, string op, bool body)
    {
        m_Uid  = uid;
        m_Op   = op;
        m_Body = body;
    }

    override void OnBody(string json)
    {
        PlayerIdentity to = OZ_ChatWho.Online(m_Uid);
        if (!to)
            return;

        OZ_ChatFail fail = new OZ_ChatFail();
        string err;
        if (JsonFileLoader<OZ_ChatFail>.LoadData(json, fail, err) && fail && fail.Error != "")
        {
            OZ_Rpc.Respond(to, OZ_PdaConst.PAGE_CHAT, m_Op, false, "", OZ_ChatFail.KeyOf(fail.Error));
            return;
        }

        // Тіло -- ПЕРЕКЛАДЕНЕ: ключі розмов стають токенами, Steam64
        // авторів -- кольорами (OZ_ChatIds.Rewrite). Як є воно не їде.
        string body = "";
        if (m_Body)
            body = OZ_ChatIds.Rewrite(m_Op, json);

        OZ_Rpc.Respond(to, OZ_PdaConst.PAGE_CHAT, m_Op, true, body, "");
    }

    override void OnFail(int code)
    {
        PlayerIdentity to = OZ_ChatWho.Online(m_Uid);
        if (!to)
            return;

        OZ_Rpc.Respond(to, OZ_PdaConst.PAGE_CHAT, m_Op, false, "", "STR_OZ_ERR_NO_BRIDGE");
    }
}

// Вхідні рядки з Discord. Один конверт -- один одержувач: міст уже розклав
// розмову по її учасниках, і сервер лише доносить.
//
// ВОРОТА ПРИЛАДУ ТУТ Є (OZ_ChatWho.SpeaksFor): увімкнений, відімкнений, жива
// сесія. Досі їх не було з доводом «list чесно віддає той самий вміст за
// запитом» -- але list на вимкненому чи замкненому приладі ворота
// сторінок не віддають, тож пуш був єдиними дверима повз замок.
class OZ_ChatSink : OZ_BridgeSink
{
    override void Deliver(string json)
    {
        OZ_ChatPush p = new OZ_ChatPush();
        string err;
        if (!JsonFileLoader<OZ_ChatPush>.LoadData(json, p, err) || !p)
        {
            OZ_Log.Warn("chat: unreadable line from the bridge: " + err);
            return;
        }

        // КОПІЇ ТУТ НЕ ТРЕБА, і це рішення, а не пропуск. OZ_ChatPush --
        // плаский клас: жодного ref-члена, самі скаляри, а корінь створює
        // скрипт рядком вище. Тому все, що приїхало, лежить у скриптовому
        // об'єкті, і Copy() для такого класу був би обрядом (шапка
        // OZ_ConfigBase; правило COPY/LEAVE -- task-57b-trap-sweep.md).
        //
        // Одержувача все ж знімаємо в локальну змінну ЗАРАЗ: нижче стоять
        // ColorFor, MakeData і new array, і читати після них зручніше з
        // рядка, який точно наш.
        string toUid = p.Uid;

        array<ref OZ_ChatHolder> tos = new array<ref OZ_ChatHolder>();
        OZ_ChatWho.Holders(toUid, tos);
        if (tos.Count() == 0)
            return;

        // Steam64 одержувача й автора на клієнт не їдуть, ключ розмови --
        // токеном (див. OZ_ChatIds).
        p.Uid = "";
        p.Id  = OZ_ChatIds.Out(p.Id);
        p.WhoColor = OZ_ChatColors.ColorFor(p.AUid);
        p.AUid = "";

        // Два варіанти конверта щонайбільше -- для худа й для екрана.
        string hudJson    = "";
        string screenJson = "";
        string eerr;

        for (int t = 0; t < tos.Count(); t++)
        {
            OZ_ChatHolder h = tos[t];
            if (!h || !h.Id)
                continue;

            string body;
            if (h.ToHud)
            {
                if (hudJson == "")
                {
                    p.ToHud = true;
                    if (!JsonFileLoader<OZ_ChatPush>.MakeData(p, hudJson, eerr, false))
                        continue;
                }
                body = hudJson;
            }
            else
            {
                if (screenJson == "")
                {
                    p.ToHud = false;
                    if (!JsonFileLoader<OZ_ChatPush>.MakeData(p, screenJson, eerr, false))
                        continue;
                }
                body = screenJson;
            }

            OZ_Rpc.Respond(h.Id, OZ_PdaConst.PAGE_CHAT, "line", true, body, "");
        }
    }

    // ЩО ЯДРО ЗНАЄ ПРО РІД "chat" -- рівно те, що сказано тут (платформа
    // §4). Ці імена жили списком у самому ядрі; задача 61 забрала їх
    // звідти, і без цих рядків читальний кеш моста стоїть порожній:
    // неоголошена дорога рахується записувальною, отже кожне відкриття
    // розмови йшло б до моста наново.
    override void Reads(array<string> routes)
    {
        routes.Insert("v1/chat/list");
        routes.Insert("v1/chat/open");
        routes.Insert("v1/chat/older");
    }

    // КУРСОР ОПИТУ -- НАШ. Міст рахує його по потоку повідомлень, тобто по
    // тому самому, з якого зроблені ці три дороги: зсув курсора застарює їх
    // навіть тоді, коли рядок був не для нас і в пачку не потрапив. Ядро
    // цього вгадати не може -- воно возить курсор, а не читає його зміст.
    override bool FollowsCursor()
    {
        return true;
    }

    // Загальне речення ядра тут майже правильне, але не називає головного:
    // вимкнене дзеркало не робить розмову недоступною -- вона лишається в
    // приладі, просто перестає їздити в Discord.
    override string MirrorNote(string kind, bool on)
    {
        if (on)
            return "the bot creates a thread per conversation and mirrors every line into Discord from the next poll";
        return "the bot stops mirroring conversations into Discord; the PDA keeps them and the threads stay as an archive";
    }
}

// -------------------------------------------------------------- сторінка

class OZ_PdaHandlerChat : OZ_PageHandler
{
    // ЧИЙ акаунт обслуговує запит -- вирішує ПРИСТРІЙ, і рішення живе
    // один виклик Handle: жива сесія на КПК говорить за свого власника,
    // хто б його не тримав. Захист -- пін, LOCK і LOG OUT OTHER DEVICES.
    private string m_Acc;
    private string m_Until;

    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok    = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";

        // «ЗАБРАТИ МІТКУ З ПОВІДОМЛЕННЯ» -- мостові не потрібна, тож і його
        // живість тут ні до чого.
        if (op == "mark_take")
            return MarkTake(json, sender, ok, error);

        // Alive(), А НЕ IsRunning() -- і це та сама помилка, яку вже ловили
        // в OZ_Link.Gated.
        //
        // IsRunning() означає «опит увімкнено», і при мертвому боті лишається
        // true назавжди. Тобто саме тоді, коли ці ворота потрібні, вони
        // пропускали: сторінка йшла по дані до моста, якого немає, і гравець
        // діставав порожній список замість відповіді «недоступно».
        //
        // Сторінка зобов'язана відрізняти «порожньо» від «не знаю» (ТЗ-2
        // R4.1). Тут живе перше з двох місць, де ця різниця вимовляється.
        if (!OZ_BridgeClient.Alive())
        {
            error = "STR_OZ_ERR_NO_BRIDGE";
            return "";
        }

        m_Acc   = sender.GetPlainId();
        m_Until = "";

        OZ_PDA_Base capDev = OZ_PdaLookup.HeldBy(sender);
        if (capDev && capDev.OZ_SessionUid() != "")
        {
            m_Acc = capDev.OZ_SessionUid();

            // КАПСУЛА -- читальня зі зрізом: розмови власника віддаються
            // станом на мить заморозки (Until ріже міст, бо правда живе в
            // Discord і зріз не кешується -- кеш згорів би з рестартом), а
            // писати не можна нічого. Капсула без штампа -- без архіву:
            // зрізати їй нема по чому.
            if (OZ_PdaCapsule.IsFrozen(capDev))
            {
                m_Until = capDev.OZ_SnapshotAt();
                if (m_Until == "" || (op != "list" && op != "open" && op != "older"))
                {
                    error = "STR_OZ_ERR_FROZEN";
                    return "";
                }
            }
            else
            {
                // ЖИВИЙ прилад у розмові -- штамп капсули йде вперед (див.
                // OZ_PDA_Base.OZ_TouchSnapshot): по ньому ріжеться історія,
                // коли прилад замерзне, і розмова, яку власник веде зараз,
                // мусить у той зріз потрапити.
                OZ_PlayerData ownPd = OZ_PlayerStore.Peek(m_Acc);
                if (ownPd)
                    capDev.OZ_TouchSnapshot(ownPd.SessionEpoch);
            }
        }

        if (op == "list")
            return List(sender, error);

        if (op == "open")
            return Open(json, sender, error);

        if (op == "older")
            return Older(json, sender, error);

        if (op == "send")
            return Send(json, sender, error);

        // start буває СИНХРОННИМ: розмова з NPC уже є в мості, і токен на неї
        // віддається одразу (StartNpc). Решта починань іде до моста й
        // повертає DEFER; відмова несе ключ. Порожня помилка тут, отже,
        // означає лише одне -- готову відповідь, і без ok=true ядро віддало б
        // її клієнтові як відмову з порожньою причиною.
        if (op == "start")
        {
            string started = Start(json, sender, error);
            if (error == "")
                ok = true;
            return started;
        }

        if (op == "group_new")
            return GroupNew(json, sender, error);

        if (op == "group_add")
            return GroupAdd(json, sender, error);

        if (op == "group_edit")
            return GroupEdit(json, sender, error);

        if (op == "group_del")
            return RefOp(json, sender, "group_del", "v1/chat/group_del", error);

        if (op == "group_leave")
            return RefOp(json, sender, "group_leave", "v1/chat/group_leave", error);

        if (op == "invite_accept")
            return RefOp(json, sender, "invite_accept", "v1/chat/invite_accept", error);

        if (op == "invite_decline")
            return RefOp(json, sender, "invite_decline", "v1/chat/invite_decline", error);

        if (op == "invitees")
            return Invitees(sender, ok, error);

        return "";
    }

    // ------------------------------------------------------------ читання

    private string List(PlayerIdentity sender, out string error)
    {
        string uid = m_Acc;
        string err;

        OZ_ChatAskMine a = new OZ_ChatAskMine();
        a.Uid = uid;
        a.Until = m_Until;

        string letter;
        if (!JsonFileLoader<OZ_ChatAskMine>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call("v1/chat/list", letter, new OZ_ChatReply(sender.GetPlainId(), "list", true));

        error = OZ_Const.DEFER;
        return "";
    }

    private string Open(string json, PlayerIdentity sender, out string error)
    {
        OZ_ChatRef r = new OZ_ChatRef();
        string err;
        if (!JsonFileLoader<OZ_ChatRef>.LoadData(json, r, err) || !r)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string uid = m_Acc;

        string key = OZ_ChatIds.In(r.Id);
        if (key == "")
        {
            error = "STR_OZ_ERR_NO_CHAT";
            return "";
        }

        OZ_ChatAskOpen a = new OZ_ChatAskOpen();
        a.Uid   = uid;
        a.Id    = key;
        a.Limit = OZ_PdaTune.ChatHistoryOpen();
        a.Until = m_Until;

        string letter;
        if (!JsonFileLoader<OZ_ChatAskOpen>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call("v1/chat/open", letter, new OZ_ChatReply(sender.GetPlainId(), "open", true));

        error = OZ_Const.DEFER;
        return "";
    }

    private string Older(string json, PlayerIdentity sender, out string error)
    {
        OZ_ChatOlderReq r = new OZ_ChatOlderReq();
        string err;
        if (!JsonFileLoader<OZ_ChatOlderReq>.LoadData(json, r, err) || !r || r.Id == "")
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string uid = m_Acc;

        string key = OZ_ChatIds.In(r.Id);
        if (key == "")
        {
            error = "STR_OZ_ERR_NO_CHAT";
            return "";
        }

        OZ_ChatAskOlder a = new OZ_ChatAskOlder();
        a.Uid    = uid;
        a.Id     = key;
        a.Before = r.Before;
        a.Limit = OZ_PdaTune.ChatHistoryPage();
        a.Until  = m_Until;

        string letter;
        if (!JsonFileLoader<OZ_ChatAskOlder>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call("v1/chat/older", letter, new OZ_ChatReply(sender.GetPlainId(), "older", true));

        error = OZ_Const.DEFER;
        return "";
    }

    // -------------------------------------------------------------- запис

    private string Send(string json, PlayerIdentity sender, out string error)
    {
        OZ_ChatSend s = new OZ_ChatSend();
        string err;
        if (!JsonFileLoader<OZ_ChatSend>.LoadData(json, s, err) || !s)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        // ДОВЖИНУ МІРЯЄМО ДО БУДЬ-ЯКОГО РІЗАННЯ, і в цьому вся правка.
        //
        // Тут стояв ванільний SanitizeString, а він -- сліпий
        // `Substring(0, 512)` по БАЙТАХ (miscgameplayfunctions.c:863). Тобто
        // повідомлення на 513-1000 байтів приїжджало сюди вже обрізаним, і
        // перевірка стелі бачила рівно 512 -- відмови не було ніколи, зате
        // хвіст зникав мовчки, а різ на 512 припадав на середину кириличної
        // пари. Стеля, яку оголошує Tuning.json, при цьому не діяла зовсім.
        //
        // Тепер міряємо те, що прислав клієнт, і відмовляємо словом
        // (ТЗ-4 R-D1.3); ріже, якщо доведеться, наш різак по межі символу.
        if (s.Text.Length() > OZ_PdaTune.ChatMsgMax())
        {
            error = "STR_OZ_ERR_MSG_TOO_LONG";
            return "";
        }

        string text = OZ_Text.Clip(s.Text, OZ_PdaTune.ChatMsgMax());
        if (text == "")
        {
            error = "STR_OZ_ERR_EMPTY_MSG";
            return "";
        }

        string uid = m_Acc;

        string key = OZ_ChatIds.In(s.Id);
        if (key == "")
        {
            error = "STR_OZ_ERR_NO_CHAT";
            return "";
        }

        OZ_ChatAskSend a = new OZ_ChatAskSend();
        a.Uid  = uid;
        a.Name = OZ_ChatWho.NameOf(uid, sender);
        a.Id   = key;
        a.Text = text;

        // Анонімність їде далі мостові, а СЛІД лишається тут: гравцям ім'я
        // не показується ніде, але власник сервера мусить мати, куди
        // подивитись після нічного погрому в ефірі.
        //
        // ДВА ІМЕНІ, а не одне. Прилад говорить за власника сесії, а набирав
        // текст той, хто його тримає: з чужим живим терміналом рядок у лозі
        // називав ЖЕРТВУ, і адмін, ідучи за ним, карав би її. Міст автора
        // анонімки не зберігає зовсім, тож інших слідів немає.
        a.Anon = s.Anon;
        if (s.Anon)
            OZ_Log.Info("chat: anonymous zone message from account " + uid + ", typed by " + sender.GetPlainId());

        string letter;
        if (!JsonFileLoader<OZ_ChatAskSend>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        // Тіла у відповіді немає навмисно: рядок з'явиться в розмові тоді,
        // коли його поверне Discord, а не коли міст підтвердить прийом.
        OZ_BridgeClient.Call("v1/chat/send", letter, new OZ_ChatReply(sender.GetPlainId(), "send", false));

        error = OZ_Const.DEFER;
        return "";
    }

    // ------------------------------------------------------- нові розмови

    private string Start(string json, PlayerIdentity sender, out string error)
    {
        OZ_NameRef r = new OZ_NameRef();
        string err;
        if (!JsonFileLoader<OZ_NameRef>.LoadData(json, r, err) || !r)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string uid = m_Acc;
        OZ_PlayerData me = OZ_PlayerStore.Peek(uid);
        if (!me)
        {
            error = "STR_OZ_ERR_NOT_CONTACT";
            return "";
        }

        // NPC-КОНТАКТ -- не друг, а пейджер. Його рядок у записнику несе тег
        // "npc:<id>", а шукали його серед хешованих ключів друзів -- тобто
        // кнопка «Написати» відповідала «не ваш контакт» завжди. Розмова з
        // NPC у мості вже є (її заводить перша ж репліка NPC), і її ключ
        // складається з тега й рахунку: сюди лише видаємо на неї токен.
        if (OZ_PdaNpc.IsNpcUid(r.Key))
            return StartNpc(r.Key, me, uid, error);

        string theirKey = UidByKeyIn(me.Friends, r.Key);
        if (theirKey == "")
        {
            // Не контакт -- писати нема кому. Саме тому контакти й заводять.
            error = "STR_OZ_ERR_NOT_CONTACT";
            return "";
        }

        // ЗАМОРОЖЕНОМУ НЕ ПИШУТЬ, і відмова тут навмисно та сама, що й для
        // будь-кого не-контакта. Пристрій того персонажа мовчить назавжди;
        // сказати про це окремими словами означало б повідомити про смерть,
        // чого КПК не робить (рішення власника 2026-08-30).
        //
        // Механічна причина не менш важлива: за цим Steam64 сьогодні живе
        // ІНША людина, і лист «старому знайомому» приїхав би саме їй.
        if (!OZ_PlayerStore.IsLive(theirKey))
        {
            error = "STR_OZ_ERR_NOT_CONTACT";
            return "";
        }

        string theirUid = OZ_PlayerStore.UidOfKey(theirKey);

        OZ_ChatAskStart a = new OZ_ChatAskStart();
        a.Uid       = uid;
        a.Name      = OZ_ChatWho.NameOf(uid, sender);
        a.OtherUid  = theirUid;
        OZ_PlayerData themPd = OZ_PlayerStore.Peek(theirUid);
        if (themPd)
            a.OtherName = themPd.Name;
        a.MyKey     = OZ_PlayerStore.KeyOf(uid);
        a.OtherKey  = theirKey;

        string letter;
        if (!JsonFileLoader<OZ_ChatAskStart>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call("v1/chat/start", letter, new OZ_ChatReply(sender.GetPlainId(), "start", true));

        error = OZ_Const.DEFER;
        return "";
    }

    private string GroupNew(string json, PlayerIdentity sender, out string error)
    {
        OZ_ChatGroupSpec r = new OZ_ChatGroupSpec();
        string err;
        if (!JsonFileLoader<OZ_ChatGroupSpec>.LoadData(json, r, err) || !r)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string title = MiscGameplayFunctions.SanitizeString(r.Name);
        if (title == "")
            title = "group";
        title = OZ_Text.Clip(title, OZ_PdaTune.ChatTitleMax());

        string uid = m_Acc;

        OZ_ChatAskGroup a = new OZ_ChatAskGroup();
        a.Uid   = uid;
        a.Title = title;
        a.Desc  = OZ_Text.Clip(MiscGameplayFunctions.SanitizeString(r.Desc), OZ_PdaTune.ChatDescMax());

        // Межа груп -- з профілю приладу засновника (ТЗ-4 R-F1.6).
        a.Max = 0;
        OZ_PDA_Base gdev = OZ_PdaLookup.HeldBy(sender);
        if (gdev)
        {
            OZ_PdaProfile gprof = OZ_PdaProfiles.ForClass(gdev.GetType());
            if (gprof && gprof.Limits)
                a.Max = gprof.Limits.GroupChats;
        }

        string letter;
        if (!JsonFileLoader<OZ_ChatAskGroup>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call("v1/chat/group_new", letter, new OZ_ChatReply(sender.GetPlainId(), "group_new", true));

        error = OZ_Const.DEFER;
        return "";
    }

    private string GroupEdit(string json, PlayerIdentity sender, out string error)
    {
        OZ_ChatGroupSpec r = new OZ_ChatGroupSpec();
        string err;
        if (!JsonFileLoader<OZ_ChatGroupSpec>.LoadData(json, r, err) || !r || r.Id == "")
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string uid = m_Acc;

        string key = OZ_ChatIds.In(r.Id);
        if (key == "")
        {
            error = "STR_OZ_ERR_NO_CHAT";
            return "";
        }

        OZ_ChatAskGroupEdit a = new OZ_ChatAskGroupEdit();
        a.Uid   = uid;
        a.Id    = key;
        a.Title = OZ_Text.Clip(MiscGameplayFunctions.SanitizeString(r.Name), OZ_PdaTune.ChatTitleMax());
        a.Desc  = OZ_Text.Clip(MiscGameplayFunctions.SanitizeString(r.Desc), OZ_PdaTune.ChatDescMax());

        string letter;
        if (!JsonFileLoader<OZ_ChatAskGroupEdit>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call("v1/chat/group_edit", letter, new OZ_ChatReply(sender.GetPlainId(), "group_edit", false));

        error = OZ_Const.DEFER;
        return "";
    }

    // Чотири операції однієї форми -- {Uid, Id} мостові, ok назад: видалення
    // групи, вихід із неї та обидві відповіді на запрошення. GroupDel була
    // копією цієї функції слово в слово, з "group_del" замість параметра.
    private string RefOp(string json, PlayerIdentity sender, string op, string route, out string error)
    {
        OZ_NoteRef r = new OZ_NoteRef();
        string err;
        if (!JsonFileLoader<OZ_NoteRef>.LoadData(json, r, err) || !r || r.Id == "")
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string key = OZ_ChatIds.In(r.Id);
        if (key == "")
        {
            error = "STR_OZ_ERR_NO_CHAT";
            return "";
        }

        OZ_ChatAskGroupDel a = new OZ_ChatAskGroupDel();
        a.Uid = m_Acc;
        a.Id  = key;

        string letter;
        if (!JsonFileLoader<OZ_ChatAskGroupDel>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call(route, letter, new OZ_ChatReply(sender.GetPlainId(), op, false));

        error = OZ_Const.DEFER;
        return "";
    }

    // Кого МОЖНА покликати -- вирішує сервер, а не текстове поле: клієнт
    // показує цей перелік і шле вибране ім'я в group_add, як і раніше.
    private string Invitees(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        // ЗАПИСНИК ВЛАСНИКА СЕСІЇ, а не того, хто тримає прилад.
        //
        // Тут стояв sender.GetPlainId(), і це розходилось із group_add на
        // сусідньому екрані: перелік «кого можна покликати» будувався з
        // контактів ТОГО, ХТО ТРИМАЄ, а покликати вдавалось лише контакта
        // ВЛАСНИКА. На захопленому чужому терміналі (а він працює як термінал
        // власника -- рішення власника 2026-08-28) список показував своїх
        // друзів, і кожен вибір із нього повертав «не ваш контакт».
        // Peek відповідає null для акаунта, якого сервер не бачив -- а Load,
        // якого цей виклик замінив, тут завжди давав дефолтний запис із
        // порожнім Friends. Порожній перелік запрошених -- та сама відповідь,
        // не внутрішня помилка: невідомий акаунт просто нікого не встиг
        // додати в друзі.
        OZ_PlayerData me = OZ_PlayerStore.Peek(m_Acc);

        OZ_ChatInvitees inv = new OZ_ChatInvitees();
        if (me)
        {
            for (int i = 0; i < me.Friends.Count(); i++)
            {
                // Заморожених у переліку немає: покликати нікуди. Вони просто
                // не з'являються серед тих, кого можна додати, -- рівно як
                // будь-хто, кого сервер не знає на ім'я.
                if (!OZ_PlayerStore.IsLive(me.Friends[i]))
                    continue;

                // Хто сховався від записників (HiddenFromContacts), того
                // немає й у переліку запрошень: записник його не показує, і
                // пікер групи не мусить бути другими дверима до тих самих
                // імен.
                OZ_PlayerData d = OZ_PlayerStore.Peek(OZ_PlayerStore.UidOfKey(me.Friends[i]));
                if (d && d.Name != "" && !d.HiddenFromContacts)
                    inv.Names.Insert(d.Name);
            }
        }

        string outJson;
        string err;
        if (!JsonFileLoader<OZ_ChatInvitees>.MakeData(inv, outJson, err, false))
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return outJson;
    }

    private string GroupAdd(string json, PlayerIdentity sender, out string error)
    {
        OZ_ChatAdd add = new OZ_ChatAdd();
        string err;
        if (!JsonFileLoader<OZ_ChatAdd>.LoadData(json, add, err) || !add)
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        string uid = m_Acc;

        // Кликати можна лише СВОГО контакта. Інакше в групу можна було б
        // затягти будь-кого, знаючи ім'я, і група стала б способом писати
        // тим, хто цього не хотів. Чи має право сам запрошувач -- звіряє
        // міст: склад розмови знає він.
        OZ_PlayerData me = OZ_PlayerStore.Peek(uid);
        if (!me)
        {
            error = "STR_OZ_ERR_NOT_CONTACT";
            return "";
        }
        string theirUid = UidByNameIn(me.Friends, add.Name);
        if (theirUid == "")
        {
            error = "STR_OZ_ERR_NOT_CONTACT";
            return "";
        }

        string key = OZ_ChatIds.In(add.Id);
        if (key == "")
        {
            error = "STR_OZ_ERR_NO_CHAT";
            return "";
        }

        OZ_ChatAskInvite a = new OZ_ChatAskInvite();
        a.Uid      = uid;
        a.Id       = key;
        a.OtherUid = theirUid;
        a.Max      = OZ_PdaTune.ChatGroupMax();
        a.TtlS     = OZ_PdaTune.GroupInviteTtlS();

        string letter;
        if (!JsonFileLoader<OZ_ChatAskInvite>.MakeData(a, letter, err, false))
        {
            OZ_Log.Error("chat: cannot build the letter: " + err);
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call("v1/chat/group_add", letter, new OZ_ChatReply(sender.GetPlainId(), "group_add", false));

        error = OZ_Const.DEFER;
        return "";
    }

    // Відповідь на «написати NPC»: токен його розмови, синхронно. Писати
    // туди не можна (розмова NPC -- лише для читання, клієнт так і малює);
    // якщо NPC ще не сказав ні слова, open чесно відповість «розмови немає».
    private string StartNpc(string tag, OZ_PlayerData me, string uid, out string error)
    {
        if (!me.NpcContacts || me.NpcContacts.Find(tag) == -1)
        {
            error = "STR_OZ_ERR_NOT_CONTACT";
            return "";
        }

        OZ_ChatRef r = new OZ_ChatRef();
        r.Id = OZ_ChatIds.Out("npc:" + OZ_PdaNpc.IdOf(tag) + ":" + uid);

        string outJson;
        string err;
        if (!JsonFileLoader<OZ_ChatRef>.MakeData(r, outJson, err, false))
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        error = "";
        return outJson;
    }

    // Мітка з повідомлення -- на карту приладу, але ВІДПОВІДЬ ЧАТУ.
    //
    // Сторінка чату слала marker_add від імені сторінки карти й тут-таки
    // писала «збережено», а справжня відповідь приходила СХОВАНІЙ сторінці
    // карти: відмова «пам'ять повна» жила в її невидимому рядку, а успіх
    // стирав ім'я, яке гравець почав набирати на карті. Тепер той самий
    // marker_add іде звідси, з перевіркою доступу до карти саме так, як її
    // робили ворота, і відповідь повертається туди, де клікнули.
    private string MarkTake(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        string why;
        if (!OZ_PageAccess.Allowed(sender, OZ_PdaConst.PAGE_MAP, "marker_add", why))
        {
            error = why;
            return "";
        }

        if (OZ_PdaCapsule.IsFrozen(OZ_PdaLookup.HeldBy(sender)))
        {
            error = "STR_OZ_ERR_FROZEN";
            return "";
        }

        return OZ_PdaHandlerMap.AddMarkerFor(json, sender, ok, error);
    }

    // ------------------------------------------------------------ дрібне

    // Розмову ПОЧИНАЮТЬ з обраного в списку -- там є ключ, і питання «хто це»
    // не стоїть.
    private string UidByKeyIn(array<string> uids, string key)
    {
        return OZ_Names.PickIn(uids, key);
    }

    // А в групу ЗАПРОШУЮТЬ за набраним ім'ям -- ключа в людини, яку щойно
    // надрукували, взятись нема звідки.
    //
    // Тому тут ім'я лишається, але з двома правилами, яких раніше не було:
    // порожнє не збігається ні з чим (інакше воно ловило б кожного, чиє ім'я
    // ще не кешоване), і двоє однакових -- це відмова, а не перший-ліпший.
    // Ім'я -> Steam64, серед СВОЇХ контактів. Список тримає ключі персонажів;
    // заморожені пропускаємо -- покликати того, кого вже немає, нікуди, а за
    // його Steam64 сьогодні живе інша людина.
    private string UidByNameIn(array<string> keys, string name)
    {
        if (name == "")
            return "";

        string found = "";

        for (int i = 0; keys && i < keys.Count(); i++)
        {
            if (!OZ_PlayerStore.IsLive(keys[i]))
                continue;

            string uid = OZ_PlayerStore.UidOfKey(keys[i]);

            OZ_PlayerData d = OZ_PlayerStore.Peek(uid);
            if (!d || d.Name != name)
                continue;

            if (found != "")
                return "";

            found = uid;
        }

        return found;
    }
}
