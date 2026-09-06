// Штампи часу ЛОКАЛЬНИМ годинником клієнта.
//
// Міст пише час у UTC, і чесніше за все показати його так, як показує
// телефон гравця: рушій дає обидва годинники (ensystem.c), різниця між
// ними і є зсув пояса цієї машини.
//
// КАЛЕНДАР ТУТ БІЛЬШЕ НЕ СВІЙ. Довжину місяця дає OZ_Time.DaysIn(рік,
// місяць) ядра, двоцифровий запис -- OZ_Time.Pad2. Свої два примірники
// («лютий завжди 28» і Two) пішли: перший уже розійшовся з ядровим і
// показував 29 лютого як 1 березня.
class OZ_LocalTime
{
    static int OffsetMin()
    {
        int lh;
        int lm;
        int ls;
        GetHourMinuteSecond(lh, lm, ls);

        int uh;
        int um;
        int us;
        GetHourMinuteSecondUTC(uh, um, us);

        int diff = (lh * 60 + lm) - (uh * 60 + um);
        if (diff > 720)
            diff -= 1440;
        if (diff < -720)
            diff += 1440;
        return diff;
    }

    // "YYYY-MM-DD HH:MM..." (UTC, T чи пробіл -- байдуже: зрізи позиційні)
    // -> "DD.MM  HH:MM" локального часу.
    //
    // РІК ТЕПЕР ЧИТАЄТЬСЯ, і без нього календар ядра покликати не можна:
    // DaysIn питає рік, щоб знати, чи лютий високосний. Чотири цифри на
    // нульовій позиції -- це те, що присилають усі три джерела цього рядка:
    // міст (ISO 8601, "2026-08-25T16:47:37.079Z") і OZ_Time.NowUtc ядра для
    // штампа знімка й дати правки записки ("2026-08-25 16:47:37").
    static string Stamp(string iso)
    {
        if (iso.Length() < 16)
            return iso;

        int y  = iso.Substring(0, 4).ToInt();
        int mo = iso.Substring(5, 2).ToInt();
        int d  = iso.Substring(8, 2).ToInt();
        int h  = iso.Substring(11, 2).ToInt();
        int mi = iso.Substring(14, 2).ToInt();

        int tot = h * 60 + mi + OffsetMin();
        if (tot >= 1440)
        {
            tot -= 1440;
            d++;
        }
        else if (tot < 0)
        {
            tot += 1440;
            d--;
        }

        if (d < 1)
        {
            mo--;
            if (mo < 1)
            {
                mo = 12;
                y--;
            }
            d = OZ_Time.DaysIn(y, mo);
        }
        else if (d > OZ_Time.DaysIn(y, mo))
        {
            d = 1;
            mo++;
            if (mo > 12)
            {
                mo = 1;
                y++;
            }
        }

        return OZ_Time.Pad2(d) + "." + OZ_Time.Pad2(mo) + "  " + OZ_Time.Pad2(tot / 60) + ":" + OZ_Time.Pad2(tot % 60);
    }
}
