// Сторінка «Новини»: список постів зліва, тіло обраного справа.
//
// Читалка для всіх -- і, з 2026-09-02, перо для лідера (ТЗ-6 R2.1). Кнопка
// WRITE з'являється лише тому, кому міст відповів "voices" з Leader або
// Admin; список імен для підпису -- теж від моста. Сторінка не вирішує
// прав і не вигадує імен (R2.3): вона малює те, що їй віддали, а маршрут
// запису перевіряє ще раз (R3.2).

class OZ_PdaPageNews : OZ_PdaPage
{
    private Widget m_Rows;
    private ref array<Widget> m_RowWgts;
    private ref OZ_NewsList m_List;
    private string m_OpenId = "";

    // Стрічка більше не влазить в один конверт: постів у базі моста стільки,
    // скільки їх написали (ТЗ-5 R-D1.1). Кнопка «ЩЕ» ходить по сторінках
    // курсором, як «старіше» в чаті, і ховається, коли міст сказав, що
    // глибше нічого немає.
    private Widget m_BtnMore;
    private string m_Next  = "";
    private bool   m_Busy  = false;

    // ОДИН "list" У ДОРОЗІ. Відповідь не каже, на який курсор вона: перша
    // сторінка, що приїхала, поки чекали «ЩЕ», докладалась як продовження
    // (верх стрічки вдруге, під тими самими рядками), а «ЩЕ», що приїхала
    // слідом, ЗАМІНЯЛА всю стрічку старшою сторінкою. Тому поки запит у
    // дорозі, новий не йде: «ЩЕ» мовчить, а «з початку» (пуш, допис,
    // повернення на вкладку) лише ставить m_Again і йде, щойно відповідь
    // (чи відмова) приїхала. Тоді кожна відповідь -- на той єдиний запит,
    // про який сторінка знає.
    private bool   m_Asking = false;
    private bool   m_Again  = false;

    // Перо лідера.
    private Widget m_Compose;
    private Widget m_BtnWrite;
    private ref array<string> m_Voices;
    private string m_Self = "";
    private int    m_Pick = 0;

    override string LayoutPath()
    {
        return "OpenZone_PDA/gui/layouts/oz_pda_page_news.layout";
    }

    override void OnBuilt()
    {
        m_Rows    = Wgt("NewsRows");
        m_RowWgts = new array<Widget>();
        m_Voices  = new array<string>();

        m_Compose  = Wgt("ComposePanel");
        m_BtnWrite = Wgt("BtnWrite");
        m_BtnMore  = Wgt("BtnMore");
        if (m_Compose)
            m_Compose.Show(false);
        if (m_BtnWrite)
            m_BtnWrite.Show(false);
        if (m_BtnMore)
            m_BtnMore.Show(false);

        SetText("BtnMoreText",      "#STR_OZ_NEWS_MORE");
        SetText("BtnWriteText",     "#STR_OZ_NEWS_WRITE");
        SetText("BtnCmpSendText",   "#STR_OZ_NEWS_SEND");
        SetText("BtnCmpCancelText", "#STR_OZ_NEWS_CANCEL");
        SetText("CmpHead",          "#STR_OZ_NEWS_COMPOSE");
    }

    override void OnSelected()
    {
        ClearHintHold();
        // З початку стрічки: сторінка відкрилась заново, і сторінки, набрані
        // минулого разу, до неї не належать. Через Restart, а не навпростець:
        // «ЩЕ», пущене до того, як вкладку перемкнули, ще може бути в дорозі.
        Restart();

        // Хто я для новин -- питаємо щоразу: грант могли зняти, поки сторінка
        // була закрита (приймання 5.9), і кнопка мусить зникнути разом із ним.
        OZ_Rpc.Request(OZ_PdaConst.PAGE_NEWS, "voices", "{}");
    }

    override void OnDeselected()
    {
        super.OnDeselected();
        if (m_Compose)
            m_Compose.Show(false);
    }

    // ДЕМОНТАЖ СТОРІНКИ: меню зносить сторінки, коли закривається чи
    // перебудовує стрічку вкладок. Рядки стрічки зареєстровані на СИНГЛТОНІ
    // WidgetEventHandler (Repaint), а база знімає лише корінь -- рядки гинуть
    // разом із ним, а записи обробника лишались би з мертвими ключами (той
    // клас помилок уже коштував нам краху клієнта). Спершу відписка, потім
    // корінь.
    override void Unlink()
    {
        // Сторінка без розкладки OnBuilt не бачила, і масиву рядків немає.
        if (m_RowWgts)
            DropRows();

        super.Unlink();
    }

    override bool OnPageClick(Widget w, int x, int y)
    {
        if (!w)
            return false;

        string nm = w.GetName();

        if (nm == "BtnWrite")
        {
            if (m_Compose)
                m_Compose.Show(true);
            // Заголовок панелі -- ще й рядок відповіді (див. Refused): на
            // відкритті він повертається до підказки.
            SetText("CmpHead", "#STR_OZ_NEWS_COMPOSE");
            m_Pick = 0;
            PaintWho();
            return true;
        }

        if (nm == "BtnCmpCancel")
        {
            if (m_Compose)
                m_Compose.Show(false);
            return true;
        }

        if (nm == "BtnCmpWho")
        {
            m_Pick++;
            if (m_Pick > m_Voices.Count())
                m_Pick = 0;
            PaintWho();
            return true;
        }

        if (nm == "BtnCmpSend")
        {
            Send();
            return true;
        }

        if (nm == "BtnMore")
        {
            // Один запит на натискання: поки сторінка не приїхала, кнопка
            // каже це й нічого не шле -- інакше три кліки дали б три однакові
            // сторінки, вставлені тричі. І не поперек першої сторінки, що
            // ще в дорозі (m_Asking): її відповідь сприйнялась би як ця.
            if (m_Asking || m_Next == "")
                return true;
            m_Busy = true;
            SetText("BtnMoreText", "#STR_OZ_CHAT_LOADING");
            AskPage(m_Next);
            return true;
        }

        // Рядок поста. Ім'я віджета -- Id треда форуму.
        if (w.GetUserID() == 7)
        {
            m_OpenId = w.GetName();

            OZ_NewsRef r = new OZ_NewsRef();
            r.Id = m_OpenId;

            string json;
            string err;
            if (JsonFileLoader<OZ_NewsRef>.MakeData(r, json, err, false))
                OZ_Rpc.Request(OZ_PdaConst.PAGE_NEWS, "open", json);

            Repaint();
            return true;
        }

        return false;
    }

    // Mouse press on a headline row: the row is a self-growing WrapSpacer
    // (see the layout's note), so OnClick never reaches it -- this is where
    // its click arrives instead (see the RegisterOnMouseButtonDown call in
    // Repaint). Left button only; the rest is not ours.
    bool OnRowDown(Widget w, int x, int y, int button)
    {
        if (button != MouseState.LEFT)
            return false;
        OZ_Log.Dbg("pda news: row pressed " + w.GetName());
        return OnPageClick(w, x, y);
    }

    // ВІДМОВУ НА ДОПИС ВИДНО ТАМ, ДЕ ЙОГО ПИСАЛИ.
    //
    // NewsHint лежить у нижньому рядку правої панелі, а панель написання
    // накриває її цілком: причина відмови малювалась ПІД оверлеєм, і лідер
    // бачив просто нічого. Тому те саме речення йде ще й у заголовок панелі,
    // який видно завжди, поки вона відкрита; BtnWrite повертає туди підказку.
    private void Refused(string said)
    {
        SetHintSticky("NewsHint", said);
        SetText("CmpHead", said);
    }

    // Одна сторінка стрічки. Курсор -- рядок моста; порожній означає
    // «найновіші» (ТЗ-5 R-D1.2).
    private void AskPage(string cursor)
    {
        OZ_NewsAskList a = new OZ_NewsAskList();
        a.Cursor = cursor;

        string json;
        string err;
        if (!JsonFileLoader<OZ_NewsAskList>.MakeData(a, json, err, false))
            return;

        m_Asking = true;
        OZ_Rpc.Request(OZ_PdaConst.PAGE_NEWS, "list", json);

        // Хто я для новин -- питаємо разом зі стрічкою лише на першій
        // сторінці: грант могли зняти, поки сторінка була закрита
        // (приймання 5.9), але «ще» про права нічого не змінює.
        if (cursor == "")
            OZ_Rpc.Request(OZ_PdaConst.PAGE_NEWS, "voices", "{}");
    }

    // Стрічку -- з початку. Поки "list" у дорозі, лише запам'ятовуємо (див.
    // m_Asking): перша сторінка піде, щойно та відповідь приїде.
    private void Restart()
    {
        if (m_Asking)
        {
            m_Again = true;
            return;
        }

        m_Next = "";
        AskPage("");
    }

    // "list" приїхав -- відповіддю, нечитним тілом чи відмовою. Дорога
    // вільна, кнопка «ЩЕ» знову каже своє, і відкладене «з початку» йде
    // тепер. Кличеться ПІСЛЯ того, як відповідь розібрано: чи це була «ЩЕ»,
    // m_Busy каже лише до цього рядка.
    private void ListLanded()
    {
        m_Asking = false;
        m_Busy = false;
        SetText("BtnMoreText", "#STR_OZ_NEWS_MORE");

        if (m_Again)
        {
            m_Again = false;
            Restart();
        }
    }

    override void OnResponse(string op, bool ok, string json, string error)
    {
        if (!ok)
        {
            // Без відповіді про імена пера не буде -- і без окремого докору:
            // причину (міст лежить) уже сказав той самий відказ на "list".
            if (op == "voices")
            {
                if (m_BtnWrite)
                    m_BtnWrite.Show(false);
                return;
            }

            if (op == "list")
                ListLanded();

            // ВІДМОВА «НЕ ТВОЯ ПЕРСОНА» НЕСЕ ПЕРЕЛІК (ТЗ-6 R1.3, приймання
            // 5.3). Сказати лідерові, що ім'я не його, і не сказати, які
            // його, -- це відповідь, після якої йдуть питати адміна.
            if (op == "post")
            {
                string said = Widget.TranslateString("#" + error);

                if (error == "STR_OZ_ERR_NEWS_NOT_YOUR_VOICE" && json != "")
                {
                    OZ_NewsFail nf = new OZ_NewsFail();
                    string ferr;
                    if (JsonFileLoader<OZ_NewsFail>.LoadData(json, nf, ferr) && nf && nf.Allowed && nf.Allowed.Count() > 0)
                    {
                        // Склеюємо ПІСЛЯ розбору й один раз: кожна склейка --
                        // виділення, а конверт розібрав серіалізатор.
                        string names = nf.Allowed[0];
                        for (int ai = 1; ai < nf.Allowed.Count(); ai++)
                            names = names + ", " + nf.Allowed[ai];
                        said = said + "  " + names;
                    }
                }

                // ЗАДОВГЕ ТІЛО -- ЧИСЛОМ ТОГО, ХТО ВІДМОВИВ (розбіжність 96).
                //
                // Той самий ключ приходить і від власної перевірки сторінки,
                // до мосту, -- у неї порожнє тіло, тож ця гілка мовчить, і
                // гравець бачить своє число тоді, коли спіткнувся об своє.
                if (error == "STR_OZ_ERR_TOO_LONG" && json != "")
                {
                    OZ_NewsFail tl = new OZ_NewsFail();
                    string terr;
                    if (JsonFileLoader<OZ_NewsFail>.LoadData(json, tl, terr) && tl && tl.Max > 0)
                        said = said + "  " + tl.Max.ToString() + " b";
                }

                Refused(said);
                return;
            }

            SetHintSticky("NewsHint", "#" + error);
            return;
        }

        string err;

        if (op == "voices")
        {
            // Не 'v': так зветься OZ_NewsView у гілці open нижче, а Enforce не
            // дає оголосити одне ім'я двічі в сусідніх гілках однієї функції.
            OZ_NewsVoices vo = new OZ_NewsVoices();
            if (!JsonFileLoader<OZ_NewsVoices>.LoadData(json, vo, err) || !vo)
                return;

            // Прапорці знімаємо ДО циклу: Insert росте, а це виділення.
            bool canWrite = false;
            if (vo.Leader || vo.Admin)
                canWrite = true;

            m_Self = vo.Self;
            m_Voices.Clear();
            if (vo.Voices)
            {
                for (int k = 0; k < vo.Voices.Count(); k++)
                    m_Voices.Insert(vo.Voices[k]);
            }
            m_Pick = 0;

            if (m_BtnWrite)
                m_BtnWrite.Show(canWrite);
            if (!canWrite && m_Compose)
                m_Compose.Show(false);
            PaintWho();
            return;
        }

        if (op == "post")
        {
            if (m_Compose)
                m_Compose.Show(false);

            EditBoxWidget te = EditBoxWidget.Cast(Wgt("CmpTitle"));
            if (te)
                te.SetText("");
            MultilineEditBoxWidget be = MultilineEditBoxWidget.Cast(Wgt("CmpBody"));
            if (be)
                be.SetText("");

            SetHint("NewsHint", "#STR_OZ_NEWS_POSTED");
            Restart();
            return;
        }

        if (op == "push")
        {
            // Свіжий пост -- перечитуємо перелік З ПОЧАТКУ: він лягає
            // зверху, і сторінки, набрані до нього, посунулись. Через
            // Restart: пуш, що впав посеред «ЩЕ», чекає на її відповідь.
            //
            // Прапорець ToHud у конверті -- справа худа (тост лише надітому
            // приладу); сторінці байдуже, звідки пуш, -- стрічка змінилась.
            Restart();
            return;
        }

        if (op == "list")
        {
            OZ_NewsList l = new OZ_NewsList();
            if (!JsonFileLoader<OZ_NewsList>.LoadData(json, l, err) || !l)
            {
                ListLanded();
                return;
            }

            // Копія: Items виділив серіалізатор, а Repaint ходить по них
            // на кожному кліку рядка.
            OZ_NewsList page = l.Copy();

            // ДОКЛАДАЄМО, а не заміняємо, якщо це продовження. Перша
            // сторінка приходить з порожнім m_Next, і тоді стрічка
            // починається наново.
            //
            // РЕСТАРТ -- ЦЕ ЗАМІНА, ХОЧ БИ ЩО ДУМАЛА КНОПКА. Курсор називав
            // допис, а допис стерли між двома натисканнями ЩЕ: міст віддає
            // найновішу сторінку й каже Restarted. Докласти її означало б
            // показати верх стрічки вдруге, під тими самими рядками, і
            // жодного способу відрізнити половини в читача немає.
            if (m_Busy && !page.Restarted && m_List && m_List.Items)
            {
                for (int pi = 0; pi < page.Items.Count(); pi++)
                    m_List.Items.Insert(page.Items[pi]);
            }
            else
            {
                m_List = page;
            }

            m_Next = page.Next;
            ListLanded();

            Repaint();
            return;
        }

        if (op == "open")
        {
            OZ_NewsView v = new OZ_NewsView();
            if (!JsonFileLoader<OZ_NewsView>.LoadData(json, v, err) || !v)
                return;

            // ВІДПОВІДЬ ПРО ІНШИЙ ДОПИС -- не наша. Кеш ядра відповідає
            // одразу, а свіжий запит іде через міст, тож порядок відповідей не
            // гарантований: пізня відповідь про A лягала під виділення B, і
            // гравець читав під одним заголовком у списку текст іншого.
            if (v.Id != m_OpenId)
                return;

            // Знімаємо все до першої склейки: SetText і Day() виділяють.
            string ptitle = v.Title;
            string pwho   = v.Who;
            string pat    = v.At;

            // КЛЕЇТЬ КЛІЄНТ (ТЗ-5 R-D1.5). Тіло приїхало масивом кусків по
            // ≤900 байтів, порізаних мостом ПО ГРАНИЦЯХ РЯДКІВ: у скрипті
            // рядок такої межі не має, і склеєне тіло малюється цілим.
            string pbody = "";
            if (v.Body)
            {
                for (int bi = 0; bi < v.Body.Count(); bi++)
                    pbody = pbody + v.Body[bi];
            }

            // Стартове повідомлення видалили в Discord (R-D1.3, H24). Прямим
            // текстом, а не порожньою панеллю: допис без пояснення читається
            // як поломка сторінки.
            if (v.Deleted && pbody == "")
                pbody = Widget.TranslateString("#STR_OZ_NEWS_DELETED");

            SetText("PostTitle", ptitle);
            SetText("PostMeta", pwho + "   " + Day(pat));

            MultilineTextWidget body = MultilineTextWidget.Cast(Wgt("PostBody"));
            if (body)
                body.SetText(pbody);

            // The spacer measures itself only on Update(): a new post's text
            // keeps the old height until the next relayout without it.
            Widget ps = Wgt("PostStack");
            if (ps)
                ps.Update();

            SetHint("NewsHint", "");
            return;
        }
    }

    // Кнопка підпису показує вибір: нуль -- своє ім'я, далі персони ГП.
    private void PaintWho()
    {
        string label;
        if (m_Pick <= 0 || m_Pick > m_Voices.Count())
        {
            m_Pick = 0;
            label = Widget.TranslateString("#STR_OZ_NEWS_AS_SELF");
            if (m_Self != "")
                label = m_Self + "  (" + label + ")";
        }
        else
        {
            label = m_Voices[m_Pick - 1];
        }
        SetText("BtnCmpWhoText", label);
    }

    private string PickedVoice()
    {
        if (m_Pick <= 0 || m_Pick > m_Voices.Count())
            return "";
        return m_Voices[m_Pick - 1];
    }

    private void Send()
    {
        string title = "";
        string body  = "";

        EditBoxWidget te = EditBoxWidget.Cast(Wgt("CmpTitle"));
        if (te)
            title = te.GetText();
        // Не сирий GetText: поле вдруковує в сам рядок \n на кожному
        // ВІЗУАЛЬНОМУ переносі, і в опублікований допис вони лягали
        // розривами посеред слів. OZ_Unwrap лишає тільки Enter-и людини --
        // той самий хід, що в записках; WrapRuler -- невидима лінійка тим
        // самим шрифтом, що й поле (розкладка сторінки).
        MultilineEditBoxWidget be = MultilineEditBoxWidget.Cast(Wgt("CmpBody"));
        if (be)
            body = OZ_Unwrap.Read(be, TextWidget.Cast(Wgt("WrapRuler")));

        // Порожнє відхиляємо на місці: сервер відповів би тим самим, але за
        // круг, і гравець чекав би на те, що бачить сам.
        if (title.Trim() == "")
        {
            Refused(Widget.TranslateString("#STR_OZ_ERR_NEWS_NO_TITLE"));
            return;
        }
        if (body.Trim() == "")
        {
            Refused(Widget.TranslateString("#STR_OZ_ERR_NEWS_NO_BODY"));
            return;
        }

        OZ_NewsPostAsk p = new OZ_NewsPostAsk();
        p.Who   = PickedVoice();
        p.Title = title;
        p.Body  = body;

        string json;
        string err;
        if (!JsonFileLoader<OZ_NewsPostAsk>.MakeData(p, json, err, false))
        {
            SetHintSticky("NewsHint", "#STR_OZ_ERR_PDA_INTERNAL");
            return;
        }

        SetHint("NewsHint", "#STR_OZ_NEWS_SENDING");
        OZ_Rpc.Request(OZ_PdaConst.PAGE_NEWS, "post", json);
    }

    // Зняти рядки стрічки -- разом з їхньою реєстрацією на синглтоні.
    private void DropRows()
    {
        for (int r = 0; r < m_RowWgts.Count(); r++)
        {
            if (m_RowWgts[r])
            {
                // The press handler was registered on the singleton in
                // Repaint: dropping the row without dropping the entry
                // leaves a stale widget key in the handler's map.
                WidgetEventHandler.GetInstance().UnregisterWidget(m_RowWgts[r]);
                m_RowWgts[r].Unlink();
            }
        }
        m_RowWgts.Clear();
    }

    private void Repaint()
    {
        DropRows();

        // The spacer measures itself only on Update(): rows added or removed
        // without it keep the old height until the next relayout.
        if (m_Rows)
            m_Rows.Update();

        int n = 0;
        if (m_List && m_List.Items)
            n = m_List.Items.Count();

        // Показуємо, коли міст сказав, що глибше є ще (ТЗ-4 R-D2.4 -- те саме
        // правило, що в чату: прапорець мусить бути фактом).
        if (m_BtnMore)
            m_BtnMore.Show(m_Next != "");

        if (n == 0)
        {
            SetHint("NewsHint", "#STR_OZ_NEWS_EMPTY");
            return;
        }

        for (int i = 0; i < n; i++)
        {
            OZ_NewsItem it = m_List.Items[i];

            Widget row = GetGame().GetWorkspace().CreateWidgets("OpenZone_PDA/gui/layouts/oz_pda_news_row.layout", m_Rows);
            if (!row)
                break;

            // A self-growing row is a WrapSpacer, not a Button, and OnClick
            // does not reach it: the press is caught here and routed the
            // same way as a chat line (OZ_PdaPageChat.LineRow).
            WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(row, this, "OnRowDown");

            row.SetName(it.Id);
            row.SetUserID(7);
            m_RowWgts.Insert(row);

            TextWidget t = TextWidget.Cast(row.FindAnyWidget("RowTitle"));
            if (t)
                t.SetText(it.Title);

            TextWidget meta = TextWidget.Cast(row.FindAnyWidget("RowMeta"));
            if (meta)
                meta.SetText(it.Who + "   " + Day(it.At));

            Widget pick = row.FindAnyWidget("RowPick");
            if (pick)
                pick.Show(it.Id == m_OpenId);
        }

        // The spacer measures itself only on Update(): rows added or removed
        // without it keep the old height until the next relayout.
        if (m_Rows)
            m_Rows.Update();
    }

    // "2026-08-28 01:23:45" -> "28.08". Різати за позиціями чесно: формат
    // задає міст і він сталий.
    private string Day(string at)
    {
        if (at.Length() < 10)
            return at;
        return at.Substring(8, 2) + "." + at.Substring(5, 2);
    }
}
