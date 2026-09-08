// Шлюз до меню для нижніх шарів.
//
// Порядок компіляції жорсткий: 3_Game -> 4_World -> 5_Mission, і знизу вгору
// видимості немає. Дія «відкрити КПК» живе в 4_World, а саме меню -- у
// 5_Mission, тож напряму покликати його вона не може.
//
// Розв'язка -- база тут і нащадок там: місія створює свого нащадка й кладе
// сюди, після чого будь-який шар зве OZ_PdaMenuGate.Open() і не знає нічого
// про UI. На виділеному сервері нащадка не існує, Open() тихо нічого не
// робить -- саме те, що треба.

class OZ_PdaMenuGate
{
    private static ref OZ_PdaMenuGate s_Inst;

    static void Bind(OZ_PdaMenuGate inst)
    {
        s_Inst = inst;
    }

    static void Open()
    {
        if (s_Inst)
            s_Inst.DoOpen();
    }

    static void Close()
    {
        if (s_Inst)
            s_Inst.DoClose();
    }

    // ТУТ БУВ ПРАПОРЕЦЬ s_Virtual із OpenVirtual/Virtual/ClearVirtual --
    // «КПК без предмета» (D132): другий привід відкрити те саме меню, дозвіл
    // адміна замість речі. Рішення власника 2026-09-08: привід один -- дія на
    // приладі в руках, -- тож і режиму, який меню мусило пам'ятати, немає.

    // Перевизначає нащадок у 5_Mission.
    void DoOpen()  { }
    void DoClose() { }
}
