// Сторінка «Новини»: стрічка форуму, що читається з КПК.
//
// ДЖЕРЕЛО ПРАВДИ -- DISCORD-ФОРУМ «новини», і пишуть туди ЛИШЕ
// адміністратори гільдії. Гра тільки читає: список постів і тіло одного
// поста. Це той випадок, де форум доречний -- вміст публічний за задумом,
// на відміну від нотатника, якому форум не підходив саме через публічність.
//
// Відповіді відкладені, як у чату й записок: міст асинхронний.

class OZ_NewsItem
{
    string Id      = "";
    string Title   = "";
    string Who     = "";
    string At      = "";
    int    Replies = 0;

    OZ_NewsItem Copy()
    {
        OZ_NewsItem c = new OZ_NewsItem();
        c.Id      = Id;
        c.Title   = Title;
        c.Who     = Who;
        c.At      = At;
        c.Replies = Replies;
        return c;
    }
}

class OZ_NewsList
{
    ref array<ref OZ_NewsItem> Items;

    // ДЕ ПРОДОВЖИТИ, і це ФАКТ, а не здогад (ТЗ-5 R-D1.1/R-D1.2). Стеля в
    // п'ятдесят постів знята: стрічка тримає все, тому вона більше не їде
    // одним конвертом. Порожній рядок означає «глибше нічого немає» --
    // міст це перевірив, а не припустив.
    string Next = "";

    void OZ_NewsList()
    {
        Items = new array<ref OZ_NewsItem>();
    }

    // Стрічку сторінка тримає в m_List і перемальовує з неї на кожному
    // кліку рядка, створюючи віджет на кожен допис.
    OZ_NewsList Copy()
    {
        OZ_NewsList c = new OZ_NewsList();
        c.Next = Next;
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

class OZ_NewsView
{
    string Id    = "";
    string Title = "";
    string Who   = "";
    string At    = "";

    // ТІЛО -- МАСИВ КУСКІВ, А НЕ РЯДОК (ТЗ-5 R-D1.4/R-D1.5).
    //
    // JsonFileLoader мовчки ріже КОЖНЕ строкове значення JSON на 1023
    // байтах при розборі -- зміряно на стенді 2026-08-28. Тобто жодне
    // одиночне поле не здатне привезти довгий допис, скільки б кусків не
    // їхало в самому конверті: стеля стоїть на розборі, а не на транспорті.
    // Масив коротких рядків здатен; міст ріже тіло по ≤900 байтів ПО
    // ГРАНИЦЯХ РЯДКІВ, а сторінка склеює при відрисовці.
    ref array<string> Body;

    // Стартове повідомлення видалили в Discord (R-D1.3). Це не те саме, що
    // допис без тексту: сторінка каже «Дані видалено» прямим текстом
    // замість порожньої панелі, яку нікому пояснити.
    bool Deleted = false;

    void OZ_NewsView()
    {
        Body = new array<string>();
    }
}

class OZ_NewsRef
{
    string Id = "";
}

class OZ_NewsFail
{
    string Error;

    // ЧИМ ЖЕ ТОДІ МОЖНА ПІДПИСАТИ (ТЗ-6 R1.3, приймання 5.3).
    //
    // Відмова «не твоя персона» без переліку доступних -- це відповідь, від
    // якої лідер іде читати вихідники. Поле знімали у фазі D саме тому, що
    // жоден ігровий тип його не оголошував; полагодити це можна лише
    // оголосивши -- ось воно.
    ref array<string> Allowed;

    void OZ_NewsFail()
    {
        Allowed = new array<string>();
    }

    // Слова моста -> ключі таблиці рядків. Міст відмовляє СЛОВАМИ (ТЗ-6 R3.3),
    // і кожне з них тут має свій переклад; невідоме слово -- «внутрішня»,
    // і в лог воно йде як є (див. OZ_NewsReply).
    static string KeyOf(string code)
    {
        if (code == "no_post")
            return "STR_OZ_ERR_NO_POST";
        if (code == "not_allowed")
            return "STR_OZ_ERR_NEWS_NOT_ALLOWED";
        if (code == "not_your_voice")
            return "STR_OZ_ERR_NEWS_NOT_YOUR_VOICE";
        if (code == "no_title")
            return "STR_OZ_ERR_NEWS_NO_TITLE";
        if (code == "no_body")
            return "STR_OZ_ERR_NEWS_NO_BODY";
        if (code == "no_author")
            return "STR_OZ_ERR_NEWS_NO_AUTHOR";
        if (code == "post_failed")
            return "STR_OZ_ERR_NEWS_POST_FAILED";
        return "STR_OZ_ERR_PDA_INTERNAL";
    }
}

class OZ_NewsReply : OZ_BridgeReply
{
    protected string m_Uid;
    protected string m_Op;

    void OZ_NewsReply(string uid, string op)
    {
        m_Uid = uid;
        m_Op  = op;
    }

    override void OnBody(string json)
    {
        PlayerIdentity to = OZ_ChatWho.Online(m_Uid);
        if (!to)
            return;

        OZ_NewsFail fail = new OZ_NewsFail();
        string err;
        if (JsonFileLoader<OZ_NewsFail>.LoadData(json, fail, err) && fail && fail.Error != "")
        {
            // Слово моста -- у лог, ключ -- гравцеві. Адмін читає лог, гравець
            // -- екран; обом потрібне своє.
            OZ_Log.Info("news: " + m_Op + " refused by the bridge: " + fail.Error);
            // Тіло відмови їде РАЗОМ із ключем: у ньому перелік доступних
            // персон (ТЗ-6 R1.3). Порожнім воно було, поки відмова несла
            // саме лише слово, і сторінці не було з чого скласти пораду.
            OZ_Rpc.Respond(to, OZ_PdaConst.PAGE_NEWS, m_Op, false, json, OZ_NewsFail.KeyOf(fail.Error));
            return;
        }

        OZ_Rpc.Respond(to, OZ_PdaConst.PAGE_NEWS, m_Op, true, json, "");
    }

    override void OnFail(int code)
    {
        PlayerIdentity to = OZ_ChatWho.Online(m_Uid);
        if (!to)
            return;

        OZ_Rpc.Respond(to, OZ_PdaConst.PAGE_NEWS, m_Op, false, "", "STR_OZ_ERR_NO_BRIDGE");
    }
}

// Лист списку: {Uid}. Раніше тут їздив лист записок -- записки відв'язано
// від моста, тож у новин тепер свій конверт.
class OZ_NewsAskList
{
    string Uid;

    // Курсор сторінки стрічки: порожній -- «найновіші». Маршрут "voices"
    // возить той самий лист і це поле просто не читає (JsonFileLoader мовчки
    // пропускає зайві ключі), тому другого класу на одне поле тут немає.
    string Cursor = "";
}

// ---- лідер пише зі свого приладу (ТЗ-6 R2.1) ----
//
// Дві операції понад читалкою: "voices" -- якими іменами цей гравець може
// підписати (список дає МІСТ, і він же вирішує, чи гравець узагалі лідер --
// R2.3), і "post" -- сам допис. Клієнт ні прав, ні імен не вигадує: він
// малює те, що йому віддали, а маршрут запису перевіряє це ще раз (R3.2).

// Що каже міст на "voices". Self -- ім'я, яким підпишеться той, хто нікого
// не вибрав; Leader/Admin -- чи є взагалі що показувати.
class OZ_NewsVoices
{
    string Self   = "";
    bool   Admin  = false;
    bool   Leader = false;
    string Org    = "";
    ref array<string> Voices;

    void OZ_NewsVoices()
    {
        Voices = new array<string>();
    }

    OZ_NewsVoices Copy()
    {
        OZ_NewsVoices c = new OZ_NewsVoices();
        c.Self   = Self;
        c.Admin  = Admin;
        c.Leader = Leader;
        c.Org    = Org;

        if (Voices)
        {
            for (int i = 0; i < Voices.Count(); i++)
                c.Voices.Insert(Voices[i]);
        }

        return c;
    }
}

// Допис. Той самий клас їде від клієнта (Uid порожній) і в міст (Uid --
// відправника, підставляє сервер: клієнт не називає, за кого просить).
class OZ_NewsPostAsk
{
    string Uid   = "";
    string Who   = "";
    string Title = "";
    string Body  = "";
}

// Свіжий пост із моста. Розголос: бачать УСІ, хто в Зоні, -- сторінка
// перечитає перелік, а тост подзвонить і тим, у кого меню закрите.
class OZ_NewsPush
{
    string Id;
    string Title;
    string Who;
    string At;
    // ЧИ ЦЕ НОВИЙ ДОПИС. Конверт їде на БУДЬ-ЯКУ зміну стрічки -- клієнт
    // скидає по ньому свій кеш list/open, -- а дзвонити мусить лише новий:
    // без цього поля виправлений чи стертий пост дзвонив у кожен КПК так
    // само, як щойно написаний.
    bool   Fresh;
}

class OZ_NewsSink : OZ_BridgeSink
{
    override void Deliver(string json)
    {
        // ТИМ, У КОГО Є ЧИМ ЧИТАТИ, а не всьому серверу.
        //
        // Конверт розсилався кожному підключеному -- разом із тими, у кого
        // КПК немає взагалі: повний JSON поста в один бік на кожну зміну
        // стрічки. Той самий фільтр, що вже стоїть на живих рядках чату
        // (OZ_ChatWho.Holders): прилад при гравці або віртуальний термінал,
        // якому адмін дозволив цю сторінку.
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

            OZ_PDA_Base dev = OZ_PdaLookup.HeldByPlayer(pl);
            if (!dev || !dev.OZ_IsOn())
            {
                if (!OZ_PdaLookup.VirtualAllows(id.GetPlainId(), OZ_PdaConst.PAGE_NEWS))
                    continue;
            }

            OZ_Rpc.Respond(id, OZ_PdaConst.PAGE_NEWS, "push", true, json, "");
        }
    }

    // ЩО ЯДРО ЗНАЄ ПРО РІД "news" -- рівно те, що сказано тут (платформа
    // §4). Ці імена жили в самому ядрі списком; задача 61 забрала їх
    // звідти, і без цих трьох рядків читальний кеш моста стоїть порожній:
    // неоголошена дорога рахується записувальною, тобто сторінка новин
    // ходила б до моста на кожне відкриття.
    override void Reads(array<string> routes)
    {
        routes.Insert("v1/news/list");
        routes.Insert("v1/news/open");
    }

    // ЯК СТРІЧКА ЗАСТАРІВАЄ -- і без цього рядка вона не кешується зовсім.
    //
    // Ядро тримає відповідь лише того роду, який сказав, ЩО ЇЇ ГАСИТЬ
    // (OZ_BridgeSink, ядро fba109d): сама Reads() -- половина контракту, і
    // такі дороги ядро кладе нейтральними, а в лог пише «"news" reads are
    // not cached». Стрічку гасить ВЛАСНИЙ конверт роду: міст шле його на
    // БУДЬ-ЯКУ зміну стрічки (news.onNews -> queuePush(..., 'news'),
    // openzone-bridge/src/index.js:1528), а Fresh лише відрізняє новий
    // допис від правки. Тому рід називає САМ СЕБЕ: інших родів він не
    // застарює, а свої list/open -- застарює кожним конвертом.
    //
    // FollowsCursor() тут була б неправдою: курсор моста рахується по
    // таблиці повідомлень (store.cursor), тобто по чату, і новини за ним
    // не їдуть.
    override void Stales(array<string> kinds)
    {
        kinds.Insert("news");
    }

    // Список персон -- питання про ПРАВА, а не про стрічку: відповідь про
    // мить, кешувати нема чого, але й гасити кешовану стрічку вона не
    // мусить. Сторінка просить список і голоси разом, і без цього список у
    // кеші не жив би довше одного запиту.
    override void Neutral(array<string> routes)
    {
        routes.Insert("v1/news/voices");
    }
}

class OZ_PdaHandlerNews : OZ_PageHandler
{
    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok    = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";

        // Alive(), а не IsRunning(): друге лишається true при мертвому боті,
        // і новини віддавали б порожній список замість «недоступно»
        // (ТЗ-2 R4.1, та сама причина, що в OZ_PdaChat).
        if (!OZ_BridgeClient.Alive())
        {
            error = "STR_OZ_ERR_NO_BRIDGE";
            return "";
        }

        string uid = sender.GetPlainId();
        string err;
        string letter;

        // "list" і "voices" -- ОДИН лист {Uid} на два маршрути; у списку до
        // нього додається курсор сторінки, який приносить клієнт.
        if (op == "voices")
            return AskUid(uid, op, "v1/news/voices", "", error);

        if (op == "list")
        {
            // Курсор -- РЯДОК МОСТА, і клієнт возить його як є: сторінка
            // отримала його з попередньої відповіді й не вигадує. Порожній
            // (або зіпсований лист) означає «з початку стрічки».
            OZ_NewsAskList want = new OZ_NewsAskList();
            string cursor = "";
            if (json != "" && JsonFileLoader<OZ_NewsAskList>.LoadData(json, want, err) && want)
                cursor = OZ_Text.Clip(want.Cursor, 128);

            return AskUid(uid, op, "v1/news/list", cursor, error);
        }

        if (op == "open")
        {
            // Розбираємо, щоб ПЕРЕВІРИТИ, і шлемо той самий документ далі:
            // ліпити з нього другий, побайтно однаковий, означало б тримати
            // два описи одного конверта.
            OZ_NewsRef r = new OZ_NewsRef();
            if (!JsonFileLoader<OZ_NewsRef>.LoadData(json, r, err) || !r || r.Id == "")
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            OZ_BridgeClient.Call("v1/news/open", json, new OZ_NewsReply(uid, "open"));
            error = OZ_Const.DEFER;
            return "";
        }

        if (op == "post")
        {
            OZ_NewsPostAsk from = new OZ_NewsPostAsk();
            if (!JsonFileLoader<OZ_NewsPostAsk>.LoadData(json, from, err) || !from)
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            // Порожнє відхиляємо тут, до мосту: круг через міст заради
            // відповіді, яку видно й так, -- зайвий.
            string title = from.Title;
            string body  = from.Body;
            if (title.Trim() == "")
            {
                error = "STR_OZ_ERR_NEWS_NO_TITLE";
                return "";
            }
            if (body.Trim() == "")
            {
                error = "STR_OZ_ERR_NEWS_NO_BODY";
                return "";
            }

            // ЗАДОВГЕ ТІЛО -- ВІДМОВА, А НЕ МОВЧАЗНИЙ ОБРУБОК (ТЗ-6 R3.3).
            //
            // Тут стояв клип: тіло, довше за стелю, їхало в гільдію
            // половиною речення, і лідер бачив свій допис обрубаним уже
            // після публікації. Панель VPP на такому ж тілі відмовляє з
            // 2026-09-06 -- дві поверхні одного роду поводились по-різному,
            // і різницю ніхто не замовляв (розбіжність 96 у звіті звірки).
            //
            // Стеля -- та сама, що вже є у приладу для нотаток
            // (Tuning.NoteBodyMaxBytes, поставочно 1000 байтів): своєї
            // другої тут заводити нема чого. Length() в Enforce байтовий.
            if (body.Length() > OZ_PdaTune.NoteBodyMax())
            {
                string said = "news: post from " + uid + " rejected, body is ";
                said = said + body.Length().ToString() + " b, ceiling is ";
                said = said + OZ_PdaTune.NoteBodyMax().ToString();
                OZ_Log.Info(said);
                error = "STR_OZ_ERR_TOO_LONG";
                return "";
            }
            if (title.Length() > OZ_PdaTune.NoteTitleMax())
            {
                error = "STR_OZ_ERR_TOO_LONG";
                return "";
            }

            // КЛІП ПЕРЕД МОСТОМ, як на кожному іншому текстовому шляху.
            //
            // Три поля з клієнта їхали в Discord як є: жодної стелі, жодної
            // чистки. Тіло, склеєне з частин RPC, обмежене лише терпінням
            // того, хто його шле, а Discord ріже своє повідомлення сам і
            // мовчки -- посеред знака. Межі беремо ті самі, що в записок
            // (заголовок і тіло).
            //
            // ПІДПИС -- ОКРЕМА РОЗМОВА (ТЗ-6 R1.2/R2.1). Who -- не текст, а
            // ІМ'Я ПЕРСОНИ, і міст звіряє його ТОЧНО зі списком, який сам
            // же й видав (personas.allowedFor). Стеля тут стояла
            // адмінська -- ChatTitleMaxBytes, 32 байти й правиться в
            // Tuning.json, -- тож персона з довшим іменем приїжджала до
            // моста обрізаною, тобто НЕ ЗБІГАЛАСЬ ІЗ ЖОДНОЮ, і лідер
            // отримував not_your_voice за ім'я, яке йому щойно запропонував
            // цей самий екран. Адмін, який опустив стелю чату до чотирьох,
            // ламав підпис новин і не мав звідки про це дізнатись.
            //
            // Тому стеля підпису -- СТАЛА й не налаштовується: вона тут
            // лише щоб підроблений RPC не привіз мегабайт.
            //
            // І це САМЕ стеля проти зловживання, а не знання про ім'я. Тут
            // писалося «Discord не дасть такого ані нікові, ані ролі» --
            // персона не є ані ніком, ані роллю: це ВІЛЬНИЙ РЯДОК, який
            // карбує /openzone persona create (openzone-bridge/src/discord.js)
            // і зберігає дослівно personas.json, БЕЗ межі з жодного боку.
            // Кому належить голос, вирішує МІСТ (ТЗ-6 R2.3), тож гра того
            // права не переміряє; 190 байтів -- це ~95 кириличних літер,
            // щедро понад будь-яке осмислене ім'я, і не більше того.
            // Заголовок і тіло вже пройшли межу вище -- клипувати їх нема
            // потреби. Підпис клипується й далі: це стеля проти підробленого
            // RPC, а не знання про ім'я персони.
            OZ_NewsPostAsk p = new OZ_NewsPostAsk();
            p.Uid   = uid;
            p.Who   = OZ_Text.Clip(from.Who, OZ_PdaConst.PERSONA_MAX_BYTES);
            p.Title = title;
            p.Body  = body;

            if (!JsonFileLoader<OZ_NewsPostAsk>.MakeData(p, letter, err, false))
            {
                error = "STR_OZ_ERR_PDA_INTERNAL";
                return "";
            }

            OZ_BridgeClient.Call("v1/news/post", letter, new OZ_NewsReply(uid, "post"));
            error = OZ_Const.DEFER;
            return "";
        }

        return "";
    }

    // Лист {Uid, Cursor} за маршрутом. Дві операції з трьох мають цю форму.
    private string AskUid(string uid, string op, string route, string cursor, out string error)
    {
        OZ_NewsAskList a = new OZ_NewsAskList();
        a.Uid    = uid;
        a.Cursor = cursor;

        string letter;
        string err;
        if (!JsonFileLoader<OZ_NewsAskList>.MakeData(a, letter, err, false))
        {
            error = "STR_OZ_ERR_PDA_INTERNAL";
            return "";
        }

        OZ_BridgeClient.Call(route, letter, new OZ_NewsReply(uid, op));
        error = OZ_Const.DEFER;
        return "";
    }
}
