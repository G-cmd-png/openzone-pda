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
}

class OZ_NewsList
{
    ref array<ref OZ_NewsItem> Items;

    void OZ_NewsList()
    {
        Items = new array<ref OZ_NewsItem>();
    }
}

class OZ_NewsView
{
    string Id    = "";
    string Title = "";
    string Who   = "";
    string At    = "";
    string Body  = "";
}

class OZ_NewsRef
{
    string Id = "";
}

class OZ_NewsFail
{
    string Error;

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

        OZ_NewsFail fail;
        string err;
        if (JsonFileLoader<OZ_NewsFail>.LoadData(json, fail, err) && fail && fail.Error != "")
        {
            // Слово моста -- у лог, ключ -- гравцеві. Адмін читає лог, гравець
            // -- екран; обом потрібне своє.
            OZ_Log.Info("news: " + m_Op + " refused by the bridge: " + fail.Error);
            OZ_Rpc.Respond(to, OZ_PdaConst.PAGE_NEWS, m_Op, false, "", OZ_NewsFail.KeyOf(fail.Error));
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

        // "list" і "voices" -- ОДИН лист {Uid} на два маршрути.
        if (op == "list" || op == "voices")
            return AskUid(uid, op, "v1/news/" + op, error);

        if (op == "open")
        {
            // Розбираємо, щоб ПЕРЕВІРИТИ, і шлемо той самий документ далі:
            // ліпити з нього другий, побайтно однаковий, означало б тримати
            // два описи одного конверта.
            OZ_NewsRef r;
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
            OZ_NewsPostAsk from;
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

            // КЛІП ПЕРЕД МОСТОМ, як на кожному іншому текстовому шляху.
            //
            // Три поля з клієнта їхали в Discord як є: жодної стелі, жодної
            // чистки. Тіло, склеєне з частин RPC, обмежене лише терпінням
            // того, хто його шле, а Discord ріже своє повідомлення сам і
            // мовчки -- посеред знака. Межі беремо ті самі, що в записок
            // (заголовок і тіло), а підпис -- коротку: це ім'я, не текст.
            OZ_NewsPostAsk p = new OZ_NewsPostAsk();
            p.Uid   = uid;
            p.Who   = OZ_Text.Clip(from.Who, OZ_PdaTune.ChatTitleMax());
            p.Title = OZ_Text.Clip(from.Title, OZ_PdaTune.NoteTitleMax());
            p.Body  = OZ_Text.Clip(from.Body, OZ_PdaTune.NoteBodyMax());

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

    // Лист {Uid} за маршрутом. Дві операції з трьох мають рівно цю форму.
    private string AskUid(string uid, string op, string route, out string error)
    {
        OZ_NewsAskList a = new OZ_NewsAskList();
        a.Uid = uid;

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
