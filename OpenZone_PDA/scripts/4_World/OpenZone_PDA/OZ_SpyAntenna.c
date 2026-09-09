// Шпигунський модуль транспондерів: бачить УСІХ, чий прилад веде, байдуже до
// обраного ними кола глядачів -- але живе лічені хвилини активної роботи
// (SpyMinutes у Hardware.json), а вичерпавшись -- згорає, як дешифратор.
//
// ДАЛЬНОСТІ ВІН НЕ ДАЄ Й НЕ ПОТРЕБУЄ: слухає він у радіусі приймача GPS, що
// стоїть у сусідньому відсіку. Своя дальність у нього була, поки він ділив
// вид зі звичайною антеною; тепер у нього свій вид ("spy") і одна робота.
//
// Ресурс живе В ПЛАТІ, не в КПК: перекинув модуль -- ресурс поїхав з ним.

class OZ_Module_SpyAntenna : ItemBase
{
    // Секунди роботи, що лишились. -1 -- плата свіжа, ще не заряджалась
    // від спеки (стеля відома лише конфігові).
    private float m_SpyLeftS = -1;

    // Списати dt секунд. true -- ресурс щойно скінчився.
    bool OZ_SpyDrain(float dt, float defaultS)
    {
        if (!GetGame().IsServer())
            return false;

        if (m_SpyLeftS < 0)
            m_SpyLeftS = defaultS;

        if (m_SpyLeftS <= 0)
            return false;

        m_SpyLeftS = m_SpyLeftS - dt;
        return m_SpyLeftS <= 0;
    }

    // СКІЛЬКИ ЛИШИЛОСЬ, у секундах. -1 -- плата ще не працювала жодної
    // секунди, і скільки в ній ресурсу, знає лише конфіг (SpyMinutes);
    // читач сам вирішує, показати повний бак чи «невідомо».
    //
    // Цей читач уже був і був знятий як мертвий код -- разом із єдиним
    // призначенням поля, ТЗ-5 R-B2.9: «екран приладу показує ОСТАЧУ
    // ресурсу кожної плати, а не саму лише її наявність». Повертається
    // РАЗОМ із рядком, який його малює (OZ_PdaModule: OZ_BayInfo.LeftMin).
    float OZ_SpyLeftS()
    {
        return m_SpyLeftS;
    }

    bool OZ_SpyAlive()
    {
        // Свіжа (-1) або з рештою ресурсу; згоріла плата й так відпаде
        // через IsRuined в OZ_ModuleClass.
        return m_SpyLeftS < 0 || m_SpyLeftS > 0;
    }

    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        auto ctx = storage["OpenZone_PDA"];
        if (!ctx)
            return;

        ctx.Write(m_SpyLeftS);
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        auto ctx = storage["OpenZone_PDA"];
        if (!ctx)
            return true;

        if (!ctx.Read(m_SpyLeftS))
            return false;

        return true;
    }
}

// Тікер шпигунської плати: щотіка списує ресурс -- поки КПК увімкнений і
// плата в гнізді.
//
// ВИД СВІЙ, а не спільний з приймачем (рішення власника 2026-09-09). Доки
// плата ділила вид "antenna" зі звичайною антеною, ця поведінка чіплялась і
// до неї -- тікер крутився на приладі, у якому шпигунської плати немає, --
// а сама антена як вид перестала існувати.
class OZ_SpyAntennaBehaviour : OZ_ModuleBehaviour
{
    override string Kind()
    {
        return OZ_PdaConst.MOD_SPY;
    }

    override string Owner()
    {
        return "OpenZone_PDA";
    }

    override float TickSeconds()
    {
        return 5;
    }

    override void OnTick(ItemBase pda, Man owner, float deltaSeconds)
    {
        OZ_PDA_Base dev = OZ_PDA_Base.Cast(pda);
        if (!dev)
            return;

        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            string cls = dev.OZ_ModuleClass(i);
            if (cls == "")
                continue;

            OZ_ModuleSpec spec = OZ_PdaHardware.ModuleFor(cls);
            if (!spec || spec.SpyMinutes <= 0)
                continue;

            OZ_Module_SpyAntenna plate = OZ_Module_SpyAntenna.Cast(dev.OZ_Attached(OZ_PdaConst.ModuleSlot(i)));
            if (!plate)
                continue;

            if (plate.OZ_SpyDrain(deltaSeconds, spec.SpyMinutes * 60))
            {
                // Ресурс вийшов -- плата згорає, слід чесний.
                plate.SetHealth("", "", 0);
                OZ_Log.Info("pda: spy antenna burnt out on " + dev.GetType());
            }
        }
    }
}
