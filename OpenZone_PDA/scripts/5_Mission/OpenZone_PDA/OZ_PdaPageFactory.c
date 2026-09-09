// Id сторінки -> її клієнтський клас.
//
// ЄДИНЕ місце, де це знання живе. Реєстр сторінок у ядрі знає, що сторінка
// існує і хто відповідає на сервері; хто малює її на клієнті -- знає лише
// цей файл.
//
// Чужий мод, що приносить свою сторінку, кладе тут свою пару через Add() зі
// свого MissionGameplay.OnInit. Тому це не switch у меню: switch довелось би
// правити щоразу, коли з'являється чужа сторінка, тобто ніколи не правити.

class OZ_PdaPageFactory
{
    private static ref map<string, typename> s_Map;

    private static void Ensure()
    {
        if (s_Map)
            return;

        s_Map = new map<string, typename>();
        Add(OZ_PdaConst.PAGE_DEVICE, OZ_PdaPageDevice);
        Add(OZ_PdaConst.PAGE_QUESTS, OZ_PdaPageQuests);
        Add(OZ_PdaConst.PAGE_CONTACTS, OZ_PdaPageContacts);
        Add(OZ_PdaConst.PAGE_NOTES, OZ_PdaPageNotes);
        Add(OZ_PdaConst.PAGE_MAP, OZ_PdaPageMap);
        Add(OZ_PdaConst.PAGE_CHAT, OZ_PdaPageChat);
        Add(OZ_PdaConst.PAGE_NEWS, OZ_PdaPageNews);
    }

    static void Add(string pageId, typename pageClass)
    {
        Ensure();
        s_Map.Set(pageId, pageClass);
    }

    // ЛІТЕРА ВКЛАДКИ -- ТЕЖ РЕЄСТР, а не сходи if-ів.
    //
    // Тут стояв перелік, у якому серед сторінок КПК лежав рядок "radio" --
    // тобто КПК знав ім'я сторінки чужого мода, якого може й не бути. Із
    // виносом фракцій таких чужих сторінок стало дві, і перелік перетворився
    // б на список усієї серії. Тепер кожен підписує свою вкладку сам.
    private static ref map<string, string> s_Glyph;

    // ВБУДОВАНІ ЛІТЕРИ СІЮТЬСЯ ЖАДІБНО, при першому дотику до реєстру -- хоч
    // з боку читача, хоч з боку того, хто дописує свою.
    //
    // Раніше вони сіялись усередині Glyph(), і тільки коли мапи ще немає. Тож
    // варто було чужому модові покликати Letter() ПЕРШИМ -- а він і кличе, з
    // OnMissionStart, який трапляється раніше за перший показ меню, -- мапа
    // ставала непорожньою, і вбудовані літери не потрапляли в неї НІКОЛИ.
    // Наслідок видно з першого погляду: на стрічці вкладок усі сім вбудованих
    // сторінок малювались знаком «?». Саме так це й виглядало на стенді
    // 2026-09-01, коли поруч став мод фракцій зі своєю літерою.
    private static void SeedGlyphs()
    {
        if (s_Glyph)
            return;

        s_Glyph = new map<string, string>();
        s_Glyph.Set(OZ_PdaConst.PAGE_DEVICE,   "D");
        s_Glyph.Set(OZ_PdaConst.PAGE_QUESTS,   "J");
        s_Glyph.Set(OZ_PdaConst.PAGE_MAP,      "M");
        s_Glyph.Set(OZ_PdaConst.PAGE_CONTACTS, "C");
        s_Glyph.Set(OZ_PdaConst.PAGE_CHAT,     "@");
        s_Glyph.Set(OZ_PdaConst.PAGE_NOTES,    "N");
        s_Glyph.Set(OZ_PdaConst.PAGE_NEWS,     "i");
    }

    static void Letter(string pageId, string glyph)
    {
        SeedGlyphs();
        s_Glyph.Set(pageId, glyph);
    }

    // ЗНАЧОК ВКЛАДКИ -- той самий реєстр, що й Letter(), з тим самим жадібним
    // посівом сімох вбудованих. Чужа сторінка, що покликала лише Letter(), у
    // цю мапу не потрапляє і отримує запасний "tab_page" -- явний запасний
    // варіант, а не рядок if-ів.
    //
    // ЗБЕРІГАЄТЬСЯ ТУТ ТЕ, ЩО ДАЛИ, а розбирається в Icon(): вбудовані сім
    // кладуть голе ім'я спрайта, чужий мод -- повне посилання на СВІЙ набір.
    private static ref map<string, string> s_Sprite;

    private static void SeedSprites()
    {
        if (s_Sprite)
            return;

        s_Sprite = new map<string, string>();
        s_Sprite.Set(OZ_PdaConst.PAGE_DEVICE,   "tab_device");
        s_Sprite.Set(OZ_PdaConst.PAGE_QUESTS,   "tab_journal");
        s_Sprite.Set(OZ_PdaConst.PAGE_MAP,      "tab_map");
        s_Sprite.Set(OZ_PdaConst.PAGE_CONTACTS, "tab_contacts");
        s_Sprite.Set(OZ_PdaConst.PAGE_CHAT,     "tab_chat");
        s_Sprite.Set(OZ_PdaConst.PAGE_NOTES,    "tab_notes");
        s_Sprite.Set(OZ_PdaConst.PAGE_NEWS,     "tab_news");
    }

    // ЗНАЧОК СТОРІНКИ ПРИНОСИТЬ ТОЙ, ХТО ПРИНІС СТОРІНКУ (рішення власника
    // 2026-09-09).
    //
    // `image` -- або ПОВНЕ посилання "set:<набір> image:<спрайт>", і тоді воно
    // йде на віджет як є, або голе ім'я спрайта, і тоді це власний набір КПК.
    // Різницю розбирає Icon(), одним поглядом на префікс.
    //
    // Раніше тут приймалось лише голе ім'я, тобто набір завжди був наш: щоб
    // мати свою вкладку, чужий мод мусив покласти свій малюнок у НАШ атлас --
    // і саме так у oz_pda_icons жили tab_radio й tab_faction, картинки модів,
    // яких у КПК немає. Атлас із чужими значками ріс би зі списком серії, а
    // мод без КПК свого значка не мав би зовсім.
    static void Sprite(string pageId, string image)
    {
        SeedSprites();
        s_Sprite.Set(pageId, image);
    }

    // ПІДПИС ВКЛАДКИ -- так само реєстр. Запасний варіант для чужої сторінки,
    // що не покликала Caption(), -- Glyph(pageId), та сама літера рельса.
    private static ref map<string, string> s_Caption;

    private static void SeedCaptions()
    {
        if (s_Caption)
            return;

        s_Caption = new map<string, string>();
        s_Caption.Set(OZ_PdaConst.PAGE_DEVICE,   "#STR_OZ_PAGE_DEVICE");
        s_Caption.Set(OZ_PdaConst.PAGE_QUESTS,   "#STR_OZ_PAGE_QUESTS");
        s_Caption.Set(OZ_PdaConst.PAGE_MAP,      "#STR_OZ_PAGE_MAP");
        s_Caption.Set(OZ_PdaConst.PAGE_CONTACTS, "#STR_OZ_PAGE_CONTACTS");
        s_Caption.Set(OZ_PdaConst.PAGE_CHAT,     "#STR_OZ_PAGE_CHAT");
        s_Caption.Set(OZ_PdaConst.PAGE_NOTES,    "#STR_OZ_PAGE_NOTES");
        s_Caption.Set(OZ_PdaConst.PAGE_NEWS,     "#STR_OZ_PAGE_NEWS");
    }

    static void Caption(string pageId, string strKey)
    {
        SeedCaptions();
        s_Caption.Set(pageId, strKey);
    }

    // ГОТОВЕ ПОСИЛАННЯ НА КАРТИНКУ ВКЛАДКИ, яке віджет читає без доробки:
    // "set:<набір> image:<спрайт>". Зареєстроване (Sprite()), або запасне
    // "tab_page" з нашого атласа, доки сторінка не принесе свого.
    //
    // Голе ім'я означає ВЛАСНИЙ набір КПК. Так пишуть усі сім вбудованих
    // сторінок, і жодна з них не мусить повторювати ім'я атласа; так само
    // лишається робочим і вже написане `Sprite(page, "tab_page")`.
    static string Icon(string pageId)
    {
        SeedSprites();

        string img = "tab_page";
        if (s_Sprite.Contains(pageId))
            img = s_Sprite.Get(pageId);

        if (img.IndexOf("set:") == 0)
            return img;

        return "set:oz_pda_icons image:" + img;
    }

    // Підпис вкладки: зареєстрований рядок КПК (Caption()), або зареєстрований
    // гліф для чужої сторінки, що покликала лише Letter().
    static string Title(string pageId)
    {
        SeedCaptions();

        if (!s_Caption.Contains(pageId))
            return Glyph(pageId);

        return s_Caption.Get(pageId);
    }

    // ПАРА СТОРІНОК, що ділять одну вкладку: ліворуч одна, праворуч друга.
    //
    // Оголошує її ТОЙ, ХТО ЇЇ УТВОРЮЄ. Раніше пару «контакти + фракція» знало
    // меню КПК -- і це означало, що КПК мусить знати ім'я фракційної сторінки
    // навіть тоді, коли мода фракцій немає.
    private static ref map<string, string> s_Pair;

    static void Pair(string pageId, string companion)
    {
        if (!s_Pair)
            s_Pair = new map<string, string>();

        s_Pair.Set(pageId, companion);
    }

    // Із ким ця сторінка ділить вкладку, або порожньо.
    static string CompanionOf(string pageId)
    {
        if (!s_Pair || !s_Pair.Contains(pageId))
            return "";

        return s_Pair.Get(pageId);
    }

    static OZ_PdaPage Make(string pageId)
    {
        Ensure();

        if (!s_Map.Contains(pageId))
        {
            // Сервер оголосив сторінку, якої клієнт малювати не вміє. Це не
            // падіння: вкладки просто не буде, а в лозі буде причина.
            OZ_Log.Warn("no client page class for \"" + pageId + "\" - tab skipped");
            return null;
        }

        OZ_PdaPage p;
        if (!Class.CastTo(p, s_Map.Get(pageId).Spawn()))
        {
            OZ_Log.Error("page class for \"" + pageId + "\" is not an OZ_PdaPage");
            return null;
        }
        return p;
    }

    // Одна літера для стрічки вкладок, поки немає власного imageset.
    static string Glyph(string pageId)
    {
        SeedGlyphs();

        if (!s_Glyph.Contains(pageId))
            return "?";

        return s_Glyph.Get(pageId);
    }
}


// Передача тексту МІЖ сторінками: карта кладе, чат забирає при відкритті.
// Статик, а не поле меню: сторінки одна одну не знають і знати не повинні.
class OZ_PdaCompose
{
    private static string s_Pending = "";

    static void Put(string text)
    {
        s_Pending = text;
    }

    static string Take()
    {
        string t = s_Pending;
        s_Pending = "";
        return t;
    }
}
