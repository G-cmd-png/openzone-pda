// Договір журналу завдань.
//
// КПК везе САМУ СТОРІНКУ й форму даних; квестовий мод везе тільки дані.
//
// Чому не «хай кожен реєструє свою сторінку»: тоді журналів стало б стільки ж,
// скільки квестових модів, кожен зі своїм виглядом, а КПК перетворився б на
// смітник вкладок. Спільний договір дає один журнал незалежно від того, чий
// мод його наповнює -- Expansion Quests, TerjeQuests, Rejects чи самописний.
//
// Постачальник підключається так: успадкувати OZ_QuestProvider, перевизначити
// Collect() і покласти себе через OZ_PdaQuests.Bind() зі свого OnMissionStart.
// Постачальник рівно один: два журнали в одному пристрої -- це знову дві
// правди. Другий Bind перезаписує перший і каже про це в лог.

// ДОГОВІР -- РІВНО ТЕ, ЩО СТОРІНКА МАЛЮЄ, і ні поля більше.
//
// Тут стояло ще ШІСТЬ: Objective.Text, Objective.Current/Total («зібрано 3
// з 5»), Entry.Summary, Entry.Giver і Entry.Position («показати на карті»).
// Жодне з них журнал не малював, постачальника, який їх заповнив би, не
// існує, а договір, що обіцяє поле й нічого з ним не робить, -- це
// обіцянка, яку перший же квестовий мод виконає й не побачить результату.
//
// П'ять пішли ревізією 2026-09-06, шосте -- Text -- лишилось і пережило
// власний коментар: сторінка рахує ним самі лише галочки («2/5»), тексту
// не показує ніде. ТЗ-5 R-B6.3 називає всі шість поіменно, тож і воно йде
// тепер (фаза E).
//
// Коли квестовий мод з'явиться, поле повертається РАЗОМ із рядком, який
// його малює -- разом із панеллю подробиць (R-B6.4): інакше воно знову
// лишиться словом у заголовку.
class OZ_QuestObjective
{
    bool   Done     = false;

    OZ_QuestObjective Copy()
    {
        OZ_QuestObjective c = new OZ_QuestObjective();
        c.Done = Done;
        return c;
    }
}

class OZ_QuestEntry
{
    string Id       = "";
    string Title    = "";
    // "active" | "done" | "failed". Рядком, а не числом: журнал показує це
    // гравцеві, а мод-постачальник не мусить знати наших констант.
    string State    = "active";
    ref array<ref OZ_QuestObjective> Objectives;

    void OZ_QuestEntry()
    {
        Objectives = new array<ref OZ_QuestObjective>();
    }

    OZ_QuestEntry Copy()
    {
        OZ_QuestEntry c = new OZ_QuestEntry();
        c.Id    = Id;
        c.Title = Title;
        c.State = State;

        if (Objectives)
        {
            for (int i = 0; i < Objectives.Count(); i++)
            {
                if (Objectives[i])
                    c.Objectives.Insert(Objectives[i].Copy());
            }
        }

        return c;
    }
}

class OZ_QuestJournal
{
    // Чи є взагалі постачальник. Порожній журнал і відсутній журнал -- різні
    // повідомлення для гравця, і плутати їх не можна.
    bool HasProvider = false;
    string ProviderName = "";
    ref array<ref OZ_QuestEntry> Entries;

    void OZ_QuestJournal()
    {
        Entries = new array<ref OZ_QuestEntry>();
    }

    // Сторінка квестів тримає журнал у m_Journal: рядки складаються в
    // Paint(), а Paint кличеться заново на кожному кліку.
    OZ_QuestJournal Copy()
    {
        OZ_QuestJournal c = new OZ_QuestJournal();
        c.HasProvider  = HasProvider;
        c.ProviderName = ProviderName;

        if (Entries)
        {
            for (int i = 0; i < Entries.Count(); i++)
            {
                if (Entries[i])
                    c.Entries.Insert(Entries[i].Copy());
            }
        }

        return c;
    }
}

class OZ_QuestProvider
{
    // Ім'я мода-постачальника. Показується в журналі, щоб гравець розумів,
    // звідки взялись завдання, коли їх кілька джерел на сервері.
    string Name()
    {
        return "unknown";
    }

    // Кличеться серверно, на запит сторінки. Заповнити journal.Entries.
    void Collect(PlayerIdentity who, OZ_QuestJournal journal)
    {
    }
}

class OZ_PdaQuests
{
    private static ref OZ_QuestProvider s_Provider;

    static void Bind(OZ_QuestProvider provider)
    {
        if (s_Provider)
        {
            string w = "quest provider replaced: " + s_Provider.Name();
            w += " -> " + provider.Name();
            OZ_Log.Warn(w);
        }
        s_Provider = provider;
        OZ_Log.Info("quest provider: " + provider.Name());
    }

    static OZ_QuestJournal Collect(PlayerIdentity who)
    {
        OZ_QuestJournal j = new OZ_QuestJournal();

        if (!s_Provider)
            return j;   // HasProvider лишається false -- сторінка так і скаже

        j.HasProvider  = true;
        j.ProviderName = s_Provider.Name();
        s_Provider.Collect(who, j);
        return j;
    }
}
