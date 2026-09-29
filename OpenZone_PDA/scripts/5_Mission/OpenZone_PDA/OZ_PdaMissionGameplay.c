// Клієнтська точка входу КПК: реєстрація меню, шлюз відкриття, опитування
// клавіші.
//
// На виділеному сервері MissionGameplay не створюється взагалі (там
// MissionServer), тож цей код туди просто не потрапляє.

class OZ_PdaMenuOpener : OZ_PdaMenuGate
{
    // Чи може гравець ЗАРАЗ дивитись у прилад: він є, живий, при тямі й не
    // зв'язаний. Те саме правило, яким сервер відсікає запити того, хто
    // просить (OZ_PdaAccess.Check), -- тут воно не пускає відкрити екран, а
    // в OZ_PdaMenu.RefreshTick закриває вже відкритий.
    static bool CanUse(PlayerBase p)
    {
        if (!p)
            return false;
        if (!p.IsAlive())
            return false;
        if (p.IsUnconscious())
            return false;
        if (p.IsRestrained())
            return false;
        return true;
    }

    override void DoOpen()
    {
        UIManager ui = GetGame().GetUIManager();
        if (!ui)
            return;

        // МЕРТВИЙ, НЕПРИТОМНИЙ ЧИ ЗВ'ЯЗАНИЙ ЕКРАНА НЕ ВІДКРИВАЄ. Сервер таким
        // відправникам відмовляє, тож вікно лише збирало б відмови, а на
        // смерті ще й тягнулося б до місії й HUD, які рушій саме розбирає, --
        // ядро ловило на цьому ACCESS_VIOLATION (OZ_LinkGate.Tick).
        if (!OZ_PdaMenuOpener.CanUse(PlayerBase.Cast(GetGame().GetPlayer())))
        {
            OZ_Log.Dbg("pda: not opening the screen - the player is missing, dead, unconscious or restrained");
            return;
        }

        // Реєстру id меню в рушії немає, тож зіткнення з ЧУЖИМ модом можливе
        // й непереборне. Єдиний захист -- не відкривати те, що вже відкрито.
        if (ui.FindMenu(OZ_PdaConst.MENU_PDA))
            return;

        // EnterScriptedMenu, а НЕ ShowScriptedMenu: друге позначає меню як
        // «створене приховано», і LockControls мовчки не спрацьовує -- ні
        // курсора, ні блокування керування.
        ui.EnterScriptedMenu(OZ_PdaConst.MENU_PDA, null);
    }

    override void DoClose()
    {
        GetGame().GetUIManager().CloseMenu(OZ_PdaConst.MENU_PDA);
    }
}

modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();

        // Ядро розносить команди «покажи це»; нас цікавить одна.
        OZ_Show.OnShow.Insert(OZ_PdaShow);

        // ВІДПОВІДІ, ЯКІ ТРЕБА ПОКАЗАТИ БЕЗ КПК.
        //
        // Обмін контактами відбувається в світі, з ЗАКРИТИМ приладом: сторінка
        // контактів у цей момент не існує й почути нічого не може. Двоє
        // стояли, тикали приладами один в одного й не бачили ані «запропоновано»,
        // ані «обмінялись» -- взагалі нічого.
        //
        // Тут -- місія, вона жива завжди. Показуємо тим самим сповіщенням,
        // яким гра говорить про все інше.
        OZ_Notice.OnAnswer.Insert(OZ_PdaNotice);

        // КЛАВІШІ ВІДКРИТТЯ БІЛЬШЕ НЕМАЄ (рішення власника 2026-09-08).
        //
        // Тут стояв OZ_PdaInput.Init(), а в OnUpdate -- Poll(): опитування
        // UAOZPdaOpen щокадру. Інпут прибрано разом із data/inputs.xml, тож
        // прив'язувати нема чого й опитувати нема кого. Екран відкриває
        // OZ_ActionOpenPda на приладі в руках, закриває Escape.
        OZ_PdaMenuGate.Bind(new OZ_PdaMenuOpener());

        // Числа худу й відсіків із пакета ядра (D87) -- на кожен пакет,
        // перший чи повторний; і одразу, якщо пакет випередив місію.
        OZ_ClientState.SyncWatch().Insert(OZ_PdaSync);
        if (OZ_ClientState.Ready())
            OZ_PdaSync(OZ_ClientState.Get());
    }

    void OZ_PdaSync(OZ_SyncPayload p)
    {
        OZ_PdaHud.ApplySync();

        // СКІЛЬКИ ВІДСІКІВ У КОЖНОГО КЛАСУ. Профілі живуть лише на сервері,
        // а видимість гнізда інвентар питає в КЛІЄНТА
        // (OZ_PDA_Base.CanDisplayAttachmentSlot): без цього рядка клієнт не
        // знав чисел зовсім і показував кожен відсік на кожному КПК. Сервер
        // кладе їх у пакет (OZ_PdaConst.SYNC_SLOTS); немає в пакеті --
        // порожній рядок, і прилад показує всі відсіки, як і досі.
        OZ_PdaProfiles.ApplyClientSlots(OZ_ClientState.Extra(OZ_PdaConst.SYNC_SLOTS, ""));
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        OZ_PdaHud.Update(timeslice);
    }

    // Сире натискання -- екранові коду, якщо КПК зараз ЗВЕРХУ. Чому саме
    // звідси, а не з OnKeyPress меню, -- в OZ_PdaMenu.OnPinKey. Лише
    // верхньому: під чужим діалогом над КПК цифри йдуть тому діалогу.
    override void OnKeyPress(int key)
    {
        super.OnKeyPress(key);

#ifndef NO_GUI
        UIManager ui = GetGame().GetUIManager();
        if (!ui)
            return;

        OZ_PdaMenu pda = OZ_PdaMenu.Cast(ui.GetMenu());
        if (pda)
            pda.OnPinKey(key);
#endif
    }

    // Мод не мав цього хука взагалі, а смужка живе прямо на робочій області
    // -- отже кожен перезапуск місії лишав по одній назавжди. Тепер знімаємо
    // за собою.
    override void OnMissionFinish()
    {
        // Дзеркало підписок з OnMissionStart: інвокери статичні й переживуть
        // місію, а місія -- ні. Слухач, що пережив свою місію, отримує
        // наступну подію вже з мертвими руками.
        OZ_Show.OnShow.Remove(OZ_PdaShow);
        OZ_Notice.OnAnswer.Remove(OZ_PdaNotice);
        OZ_ClientState.SyncWatch().Remove(OZ_PdaSync);

        // Числа відсіків -- статик, а статики переживають місію: наступний
        // сервер не мусить успадкувати наші, поки не пришле свої.
        OZ_PdaProfiles.ForgetClientSlots();

        OZ_PdaHud.Teardown();
        super.OnMissionFinish();
    }

    // Показуємо лише те, що стосується світу, а не меню: коли КПК відкритий,
    // сторінка контактів скаже те саме своєю підказкою, і два повідомлення про
    // одне гірші за одне.
    void OZ_PdaNotice(string op, bool ok, string why)
    {
        if (op != "swap")
            return;

        NotificationSystem.AddNotificationExtended(4, "#STR_OZ_PDA_NAME", OZ_Notice.Text(), "");
    }

    void OZ_PdaShow(string what)
    {
        if (what == "pda")
            OZ_PdaMenuGate.Open();

        // Тут стояла гілка "pda_virtual" -- екран без предмета за ним (D132).
        // Рішення власника 2026-09-08: сервер такої команди більше не шле, і
        // приймати її нема кому.
    }

    override UIScriptedMenu CreateScriptedMenu(int id)
    {
        // super ПЕРШИЙ і вихід одразу, якщо він щось віддав: саме це тримає
        // сумісність з іншими модами, що чіпали той самий клас.
        UIScriptedMenu menu = super.CreateScriptedMenu(id);
        if (menu)
            return menu;

#ifndef NO_GUI
        if (id == OZ_PdaConst.MENU_PDA)
        {
            menu = new OZ_PdaMenu();
            menu.SetID(id);
        }

        if (id == OZ_PdaConst.MENU_PDA_HUD)
        {
            menu = new OZ_HudEditMenu();
            menu.SetID(id);
        }
#endif

        return menu;
    }
}
