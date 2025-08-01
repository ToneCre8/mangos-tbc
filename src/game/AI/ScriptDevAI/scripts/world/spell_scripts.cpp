/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Spell_Scripts
SD%Complete: 100
SDComment: Spell scripts for dummy effects (if script need access/interaction with parts of other AI or instance, add it in related source files)
SDCategory: Spell
EndScriptData */

/* ContentData
spell 21014
spell 21050
spell 26275
spell 29528
spell 29866
spell 34665
spell 37136
spell 39246
spell 44935
spell 45109
spell 45111
EndContentData */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "Spells/Scripts/SpellScript.h"
#include "Grids/Cell.h"
#include "Grids/CellImpl.h"
#include "Grids/GridNotifiers.h"
#include "Grids/GridNotifiersImpl.h"

/* When you make a spell effect:
- always check spell id and effect index
- always return true when the spell is handled by script
*/

enum
{
    // quest 9452
    SPELL_CAST_FISHING_NET      = 29866,
    GO_RED_SNAPPER              = 181616,
    NPC_ANGRY_MURLOC            = 17102,
    ITEM_RED_SNAPPER            = 23614,
    SPELL_FISHED_UP_MURLOC      = 29869,
    SPELL_FISHED_UP_RED_SNAPPER = 29867,
    // SPELL_SUMMON_TEST           = 49214                  // ! Just wrong spell name? It summon correct creature (17102)but does not appear to be used.
};

// 29866 - Cast Fishing Net
struct CastFishingNet : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        GameObject* goTarget = spell->GetGOTarget();
        Unit* caster = spell->GetCaster();
        if (goTarget->GetRespawnTime() != 0 || !caster->IsPlayer())
            return;

        if (urand(0, 2))
            caster->CastSpell(nullptr, SPELL_FISHED_UP_MURLOC, TRIGGERED_OLD_TRIGGERED);
        else
            caster->CastSpell(nullptr, SPELL_FISHED_UP_RED_SNAPPER, TRIGGERED_OLD_TRIGGERED);
    }
};

enum
{
    // target hulking helboar
    SPELL_ADMINISTER_ANTIDOTE           = 34665,
    NPC_HELBOAR                         = 16880,
    NPC_DREADTUSK                       = 16992,

    // quest 11515
    SPELL_FEL_SIPHON_DUMMY              = 44936,
    NPC_FELBLOOD_INITIATE               = 24918,
    NPC_EMACIATED_FELBLOOD              = 24955,

    // target nestlewood owlkin
    SPELL_INOCULATE_OWLKIN              = 29528,
    NPC_OWLKIN                          = 16518,
    NPC_OWLKIN_INOC                     = 16534,

    // quest 9849, item 24501
    SPELL_THROW_GORDAWG_BOULDER         = 32001,
    NPC_MINION_OF_GUROK                 = 18181,

    // quest 6661
    SPELL_MELODIOUS_RAPTURE             = 21050,
    SPELL_MELODIOUS_RAPTURE_VISUAL      = 21051,
    NPC_DEEPRUN_RAT                     = 13016,
    NPC_ENTHRALLED_DEEPRUN_RAT          = 13017,
};

bool EffectDummyCreature_spell_dummy_npc(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Creature* pCreatureTarget, ObjectGuid /*originalCasterGuid*/)
{
    switch (uiSpellId)
    {
        case SPELL_ADMINISTER_ANTIDOTE:
        {
            if (uiEffIndex == EFFECT_INDEX_0)
            {
                if (pCreatureTarget->GetEntry() != NPC_HELBOAR)
                    return true;

                // possible needs check for quest state, to not have any effect when quest really complete

                pCreatureTarget->UpdateEntry(NPC_DREADTUSK);
                return true;
            }
            return true;
        }
        case SPELL_INOCULATE_OWLKIN:
        {
            if (uiEffIndex == EFFECT_INDEX_0)
            {
                if (pCreatureTarget->GetEntry() != NPC_OWLKIN)
                    return true;

                pCreatureTarget->UpdateEntry(NPC_OWLKIN_INOC);
                pCreatureTarget->AIM_Initialize();
                ((Player*)pCaster)->KilledMonsterCredit(NPC_OWLKIN_INOC);

                // set despawn timer, since we want to remove creature after a short time
                pCreatureTarget->ForcedDespawn(15000);

                return true;
            }
            return true;
        }
        case SPELL_FEL_SIPHON_DUMMY:
        {
            if (uiEffIndex == EFFECT_INDEX_0)
            {
                if (pCreatureTarget->GetEntry() != NPC_FELBLOOD_INITIATE)
                    return true;

                pCreatureTarget->UpdateEntry(NPC_EMACIATED_FELBLOOD);
                return true;
            }
            return true;
        }
        case SPELL_THROW_GORDAWG_BOULDER:
        {
            if (uiEffIndex == EFFECT_INDEX_0)
            {
                for (int i = 0; i < 3; ++i)
                {
                    if (irand(i, 2))                        // 2-3 summons
                        pCreatureTarget->SummonCreature(NPC_MINION_OF_GUROK, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSPAWN_CORPSE_DESPAWN, 5000);
                }

                pCreatureTarget->CastSpell(nullptr, 3617, TRIGGERED_OLD_TRIGGERED); // suicide spell
                return true;
            }
            return true;
        }
        case SPELL_MELODIOUS_RAPTURE:
        {
            if (uiEffIndex == EFFECT_INDEX_0)
            {
                if (pCaster->GetTypeId() != TYPEID_PLAYER && pCreatureTarget->GetEntry() != NPC_DEEPRUN_RAT)
                    return true;

                pCreatureTarget->UpdateEntry(NPC_ENTHRALLED_DEEPRUN_RAT);
                pCreatureTarget->CastSpell(pCreatureTarget, SPELL_MELODIOUS_RAPTURE_VISUAL, TRIGGERED_NONE);
                pCreatureTarget->GetMotionMaster()->MoveFollow(pCaster, frand(0.5f, 3.0f), frand(M_PI_F * 0.8f, M_PI_F * 1.2f));

                ((Player*)pCaster)->KilledMonsterCredit(NPC_ENTHRALLED_DEEPRUN_RAT);
            }
            return true;
        }
    }

    return false;
}

struct GreaterInvisibilityMob : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        if (apply)
            aura->ForcePeriodicity(1 * IN_MILLISECONDS); // tick every second
    }

    void OnPeriodicTickEnd(Aura* aura) const override
    {
        Unit* target = aura->GetTarget();
        if (!target->IsCreature())
            return;

        Creature* invisible = static_cast<Creature*>(target);
        std::list<Unit*> nearbyTargets;
        MaNGOS::AnyUnitInObjectRangeCheck u_check(invisible, float(invisible->GetDetectionRange()));
        MaNGOS::UnitListSearcher<MaNGOS::AnyUnitInObjectRangeCheck> searcher(nearbyTargets, u_check);
        Cell::VisitWorldObjects(invisible, searcher, invisible->GetDetectionRange());
        for (Unit* nearby : nearbyTargets)
        {
            if (invisible->CanAttackOnSight(nearby) && invisible->IsWithinLOSInMap(nearby, true))
            {
                invisible->AI()->AttackStart(nearby);
                return;
            }
        }
    }
};

struct InebriateRemoval : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        Unit* target = aura->GetTarget();
        if (!target->IsPlayer())
            return;

        SpellEffectIndex effIdx;
        SpellEffectIndex effIdxInebriate;
        switch (aura->GetSpellProto()->Id)
        {
            case 29690: effIdx = EFFECT_INDEX_1; effIdxInebriate = EFFECT_INDEX_2; break;
            case 37591: effIdx = EFFECT_INDEX_0; effIdxInebriate = EFFECT_INDEX_1; break;
            default: return;
        }
        Player* player = static_cast<Player*>(target);
        if (!apply && aura->GetEffIndex() == effIdx)
            player->SetDrunkValue(uint16(std::max(int32(player->GetDrunkValue()) - player->CalculateSpellEffectValue(player, aura->GetSpellProto(), effIdxInebriate) * 256, 0)));
    }
};

struct AstralBite : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        if (Unit* caster = spell->GetCaster())
            caster->getThreatManager().modifyAllThreatPercent(-100);
    }
};

struct FelInfusion : public SpellScript
{
    void OnInit(Spell* spell) const override
    {
        spell->SetMaxAffectedTargets(1);
        spell->SetFilteringScheme(EFFECT_INDEX_0, true, SCHEME_CLOSEST);
    }

    bool OnCheckTarget(const Spell* /*spell*/, Unit* target, SpellEffectIndex /*eff*/) const override
    {
        if (!target->IsInCombat())
            return false;
        return true;
    }
};

enum
{
    SPELL_POSSESS       = 32830,
    SPELL_POSSESS_BUFF  = 32831,
    SPELL_POSSESS_INVIS = 32832,
    SPELL_KNOCKDOWN     = 13360,
};

struct AuchenaiPossess : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        if (apply)
        {
            Unit* caster = aura->GetCaster();
            if (caster)
                caster->CastSpell(nullptr, SPELL_POSSESS_INVIS, TRIGGERED_OLD_TRIGGERED);
            aura->GetTarget()->CastSpell(nullptr, SPELL_POSSESS_BUFF, TRIGGERED_OLD_TRIGGERED);
            aura->ForcePeriodicity(1000);
        }
        else
        {
            aura->GetTarget()->RemoveAurasDueToSpell(SPELL_POSSESS_BUFF);
            aura->GetTarget()->CastSpell(aura->GetTarget(), SPELL_KNOCKDOWN, TRIGGERED_OLD_TRIGGERED);
            if (Unit* caster = aura->GetCaster())
                if (caster->IsCreature())
                    static_cast<Creature*>(caster)->ForcedDespawn();
        }
    }

    void OnPeriodicTickEnd(Aura* aura) const override
    {
        if (aura->GetTarget()->GetHealthPercent() < 50.f)
            aura->GetTarget()->RemoveAurasDueToSpell(SPELL_POSSESS);
    }
};

struct GettingSleepyAura : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        if (!apply && aura->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
            aura->GetTarget()->CastSpell(nullptr, 34801, TRIGGERED_OLD_TRIGGERED); // Sleep
    }
};

struct AllergiesAura : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        if (apply)
            aura->ForcePeriodicity(10 * IN_MILLISECONDS);
    }

    void OnPeriodicDummy(Aura* aura) const override
    {
        if (urand(0, 2) > 0)
            aura->GetTarget()->CastSpell(nullptr, 31428, TRIGGERED_OLD_TRIGGERED); // Sneeze
    }
};

enum
{
    SPELL_USE_CORPSE = 33985,
};

struct RaiseDead : public SpellScript
{
    void OnInit(Spell* spell) const override
    {
        spell->SetMaxAffectedTargets(1);
    }

    bool OnCheckTarget(const Spell* /*spell*/, Unit* target, SpellEffectIndex /*eff*/) const override
    {
        if (!target->IsCreature() || static_cast<Creature*>(target)->HasBeenHitBySpell(SPELL_USE_CORPSE))
            return false;

        return true;
    }
};

struct UseCorpse : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (!target || !target->IsCreature())
            return;

        static_cast<Creature*>(target)->RegisterHitBySpell(SPELL_USE_CORPSE);
    }
};

struct SplitDamage : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (spell->m_spellInfo->Effect[effIdx] != SPELL_EFFECT_SCHOOL_DAMAGE)
            return;

        uint32 count = 0;
        auto& targetList = spell->GetTargetList();
        for (Spell::TargetList::const_iterator ihit = targetList.begin(); ihit != targetList.end(); ++ihit)
            if (ihit->effectHitMask & (1 << effIdx))
                ++count;

        spell->SetDamage(spell->GetDamage() / count); // divide to all targets
    }
};

struct TKDive : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (spell->m_spellInfo->Effect[effIdx] != SPELL_EFFECT_SCHOOL_DAMAGE)
            return;

        Unit* target = spell->GetUnitTarget();
        spell->GetCaster()->AddThreat(target, 1000000.f);
    }
};

struct CurseOfPain : public AuraScript
{
    void OnPeriodicTickEnd(Aura* aura) const override
    {
        if (aura->GetTarget()->GetHealthPercent() < 50.f)
            aura->GetTarget()->RemoveAurasDueToSpell(aura->GetId());
    }
};

enum SeedOfCorruptionNpc
{
    SPELL_SEED_OF_CORRUPTION_PROC_DEFAULT   = 32865,
    SPELL_SEED_OF_CORRUPTION_NPC_24558      = 44141,
    SPELL_SEED_OF_CORRUPTION_PROC_NPC_24558 = 43991,
};

struct spell_seed_of_corruption_npc : public AuraScript
{
    SpellAuraProcResult OnProc(Aura* aura, ProcExecutionData& procData) const override
    {
        if (aura->GetEffIndex() != EFFECT_INDEX_1)
            return SPELL_AURA_PROC_OK;
        Modifier* mod = procData.triggeredByAura->GetModifier();
        // if damage is more than need deal finish spell
        if (mod->m_amount <= (int32)procData.damage)
        {
            // remember guid before aura delete
            ObjectGuid casterGuid = procData.triggeredByAura->GetCasterGuid();

            int32 basePoints = 2000; // guesswork, need to fill for all spells that use this because its not in spell data

            // Remove aura (before cast for prevent infinite loop handlers)
            procData.victim->RemoveAurasByCasterSpell(procData.triggeredByAura->GetId(), procData.triggeredByAura->GetCasterGuid());

            // Cast finish spell (triggeredByAura already not exist!)
            uint32 triggered_spell_id = 0;
            switch (aura->GetSpellProto()->Id)
            {
                case SPELL_SEED_OF_CORRUPTION_NPC_24558: triggered_spell_id = SPELL_SEED_OF_CORRUPTION_PROC_NPC_24558; break;
                default: triggered_spell_id = SPELL_SEED_OF_CORRUPTION_PROC_DEFAULT; break;
            }
            if (Unit* caster = procData.triggeredByAura->GetCaster())
                caster->CastCustomSpell(procData.victim, triggered_spell_id, &basePoints, nullptr, nullptr, TRIGGERED_OLD_TRIGGERED);

            return SPELL_AURA_PROC_OK;              // no hidden cooldown
        }

        // Damage counting
        mod->m_amount -= procData.damage;
        return SPELL_AURA_PROC_OK;
    }
};

/* *****************************
*  PX-238 Winter Wondervolt TRAP
*******************************/
struct WondervoltTrap : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx == EFFECT_INDEX_0)
        {
            if (Unit* target = spell->GetUnitTarget())
            {
                if (WorldObject* source = spell->GetCastingObject())
                    if (!source->IsWithinDist(target, 1.0f))
                        return;

                if (spell->GetUnitTarget()->getGender() == GENDER_MALE)
                {
                    target->RemoveAurasDueToSpell(26157);
                    target->RemoveAurasDueToSpell(26273);
                    target->CastSpell(target, urand(0, 1) ? 26157 : 26273, TRIGGERED_OLD_TRIGGERED);
                }
                else
                {
                    target->RemoveAurasDueToSpell(26272);
                    target->RemoveAurasDueToSpell(26274);
                    target->CastSpell(target, urand(0, 1) ? 26272 : 26274, TRIGGERED_OLD_TRIGGERED);
                }
            }
            return;
        }
    }
};

/* ************************************************************
*  Arcane Cloaking
*  Quests 9121, 9122, 9123, 9378 - Naxxramas, The Dread Citadel
**************************************************************/

// 28006 - Arcane Cloaking
struct ArcaneCloaking : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx == EFFECT_INDEX_0)
        {
            Unit* caster = spell->GetCaster();
            Unit* target = spell->GetUnitTarget();
            // Naxxramas Entry Flag Effect DND
            if (target && target->IsPlayer())
                caster->CastSpell(target, 29296, TRIGGERED_OLD_TRIGGERED);  // Cast Naxxramas Entry Flag Trigger DND
        }
    }
};

enum SpellVisualKitFoodOrDrink
{
    SPELL_VISUAL_KIT_FOOD = 406,
    SPELL_VISUAL_KIT_DRINK = 438
};

struct FoodAnimation : public AuraScript
{
    void OnHeartbeat(Aura* aura) const override
    {
        aura->GetTarget()->PlaySpellVisual(SPELL_VISUAL_KIT_FOOD);
    }
};

struct DrinkAnimation : public AuraScript
{
    void OnHeartbeat(Aura* aura) const override
    {
        aura->GetTarget()->PlaySpellVisual(SPELL_VISUAL_KIT_DRINK);
    }
};

struct Drink : public DrinkAnimation
{
    void OnApply(Aura* aura, bool apply) const override
    {
        if (!apply || aura->GetEffIndex() != EFFECT_INDEX_0)
            return;

        if (!aura->GetTarget()->IsPlayer())
            return;

        if (aura->GetTarget()->GetMap()->IsBattleArena())
            return;

        if (Aura* periodicAura = aura->GetHolder()->GetAuraByEffectIndex((SpellEffectIndex)(aura->GetEffIndex() + 1)))
            aura->GetModifier()->m_amount = periodicAura->GetModifier()->m_amount;
    }

    void OnPeriodicDummy(Aura* aura) const override
    {
        if (aura->GetEffIndex() != EFFECT_INDEX_1)
            return;

        if (!aura->GetTarget()->IsPlayer())
            return;

        if (!aura->GetTarget()->GetMap()->IsBattleArena())
            return;

        //if (aura->GetAuraTicks() != 2) // todo: wait for 2nd tick to update regen in Arena only? (needs confirmation)
        //    return;

        aura->ForcePeriodicity(0);

        if (Aura* regenAura = aura->GetHolder()->GetAuraByEffectIndex((SpellEffectIndex)(aura->GetEffIndex() - 1)))
        {
            regenAura->GetModifier()->m_amount = aura->GetModifier()->m_amount;
            ((Player*)aura->GetTarget())->UpdateManaRegen();
        }
    }
};

struct spell_effect_summon_no_follow_movement : public SpellScript
{
    void OnSummon(Spell* /*spell*/, Creature* summon) const override
    {
        summon->AI()->SetFollowMovement(false);
    }
};

struct SpellHasteHealerTrinket : public AuraScript
{
    bool OnCheckProc(Aura* /*aura*/, ProcExecutionData& data) const override
    {
        // should only proc off of direct heals or HoT applications
        if (data.spell && (data.isHeal || IsSpellHaveAura(data.spellInfo, SPELL_AURA_PERIODIC_HEAL)))
            return true;

        return false;
    }
};

struct IncreasedHealingDoneDummy : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        aura->GetTarget()->RegisterScriptedLocationAura(aura, SCRIPT_LOCATION_SPELL_HEALING_DONE, apply);
    }

    void OnDamageCalculate(Aura* aura, Unit* /*attacker*/, Unit* /*victim*/, int32& advertisedBenefit, float& /*totalMod*/) const override
    {
        advertisedBenefit += aura->GetModifier()->m_amount;
    }
};

struct IncreasedSpellDamageDoneDummy : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        aura->GetTarget()->RegisterScriptedLocationAura(aura, SCRIPT_LOCATION_SPELL_DAMAGE_DONE, apply);
    }

    void OnDamageCalculate(Aura* aura, Unit* /*attacker*/, Unit* /*victim*/, int32& advertisedBenefit, float& /*totalMod*/) const override
    {
        advertisedBenefit += aura->GetModifier()->m_amount;
    }
};

struct spell_scourge_strike : public SpellScript
{
    bool OnCheckTarget(const Spell* /*spell*/, Unit* target, SpellEffectIndex /*eff*/) const override
    {
        if (target->IsPlayer() || (target->IsPlayerControlled()))
            return false;

        return true;
    }
};

enum
{
    SPELL_THISTLEFUR_DEATH = 8603,
    SPELL_RIVERPAW_DEATH   = 8655,
    SPELL_STROMGARDE_DEATH = 8894,
    SPELL_CRUSHRIDGE_DEATH = 9144,

    SAY_RAGE_FALLEN        = 1151,
};

struct TribalDeath : public SpellScript
{
    bool OnCheckTarget(const Spell* spell, Unit* target, SpellEffectIndex /*eff*/) const override
    {
        uint32 entry = 0;
        switch (spell->m_spellInfo->Id)
        {
            case SPELL_THISTLEFUR_DEATH: entry = 3925; break; // Thistlefur Avenger
            case SPELL_RIVERPAW_DEATH: entry = 0; break; // Unk
            case SPELL_STROMGARDE_DEATH: entry = 2585; break; // Stromgarde Vindicator
            case SPELL_CRUSHRIDGE_DEATH: entry = 2287; break; // Crushridge Warmonger
        }
        if (target->GetEntry() != entry)
            return false;
        return true;
    }

    void OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        uint32 spellId = 0;
        switch (spell->m_spellInfo->Id)
        {
            case SPELL_THISTLEFUR_DEATH: spellId = 8602; break;
            case SPELL_RIVERPAW_DEATH: spellId = 0; break; // Unk
            case SPELL_STROMGARDE_DEATH: spellId = 8602; break;
            case SPELL_CRUSHRIDGE_DEATH: spellId = 8269; break;
        }
        Unit* target = spell->GetUnitTarget();
        Unit* caster = spell->GetCaster();
        target->CastSpell(nullptr, spellId, TRIGGERED_OLD_TRIGGERED);
        if (!target->IsInCombat())
            if (Unit* killer = target->GetMap()->GetUnit(static_cast<Creature*>(target)->GetKillerGuid()))
                target->AI()->AttackStart(killer);

        if (spell->m_spellInfo->Id == SPELL_CRUSHRIDGE_DEATH)
            DoBroadcastText(SAY_RAGE_FALLEN, target, caster);
    }
};

struct RetaliationCreature : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        if (!spell->m_targets.getUnitTarget() || !spell->GetCaster()->HasInArc(spell->m_targets.getUnitTarget()))
            return SPELL_FAILED_CASTER_AURASTATE;

        return SPELL_CAST_OK;
    }
};

// 11920 - Net Guard
struct NetGuard : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        spell->GetCaster()->getThreatManager().modifyThreatPercent(spell->GetUnitTarget(), -50);
    }
};

struct HateToHalf : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        spell->GetCaster()->getThreatManager().modifyThreatPercent(spell->GetUnitTarget(), -50);
    }
};

struct HateToZero : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        spell->GetCaster()->getThreatManager().modifyThreatPercent(spell->GetUnitTarget(), -100);
    }
};

struct PreventSpellIfSameAuraOnCaster : public SpellScript
{
    SpellCastResult OnCheckCast(Spell* spell, bool /*strict*/) const override
    {
        if (spell->GetCaster()->HasAura(spell->m_spellInfo->Id))
            return SPELL_FAILED_CASTER_AURASTATE;

        return SPELL_CAST_OK;
    }
};

struct InstillLordValthalaksSpirit : public SpellScript
{
    void OnSummon(Spell* spell, Creature* summon) const override
    {
        spell->GetCaster()->AddCreature(spell->m_spellInfo->Id, summon);
    }
};

struct Stoned : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        Unit* target = aura->GetTarget();
        if (apply)
        {
            if (target->GetTypeId() != TYPEID_UNIT)
                return;

            if (target->GetEntry() == 25507)
                return;

            target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PLAYER | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_UNINTERACTIBLE);
            target->addUnitState(UNIT_STAT_ROOT);
        }
        else
        {
            if (target->GetTypeId() != TYPEID_UNIT)
                return;

            if (target->GetEntry() == 25507)
                return;

            // see dummy effect of spell 10254 for removal of flags etc
            target->CastSpell(nullptr, 10254, TRIGGERED_OLD_TRIGGERED);
        }
    }
};

struct BirthNoVisualInstantSpawn : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        spell->GetCaster()->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DO_NOT_FADE_IN);
    }
};

struct SleepVisualFlavor : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        Unit* target = aura->GetTarget();
        if (apply)
            target->SetStandState(UNIT_STAND_STATE_SLEEP);
        else
            target->SetStandState(UNIT_STAND_STATE_STAND);
    }
};

enum spell_call_of_the_falcon
{
    YELL_KILL_FALCONER          = 17624, // Kill $n!
    NPC_BLOODWARDER_FALCONER    = 17994,
    NPC_BLOODFALCON             = 18155,
    SPELL_CALL_OF_THE_FALCON    = 34853,
};

struct CallOfTheFalcon : public AuraScript
{
    void OnApply(Aura* aura, bool apply) const override
    {
        if (apply)
        {
            DoBroadcastText(YELL_KILL_FALCONER, aura->GetCaster(), aura->GetTarget());
            aura->GetTarget()->CastSpell(nullptr, SPELL_CALL_OF_THE_FALCON, TRIGGERED_OLD_TRIGGERED);
        }
    }
};

struct MaximizePetLoyalty : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* unitTarget = spell->GetUnitTarget();
        if (!unitTarget)
            return;

        Pet* pet = dynamic_cast<Pet*>(unitTarget);

        if (!pet)
            return;

        if (pet->getPetType() != HUNTER_PET)
            return;

        pet->SetLoyaltyLevel(LoyaltyLevel(6));
        pet->SetTP(300);
    }
};


// 36435 - Forget                                                               // Unlearn Armorsmith specialization
struct ForgetArmorsmith : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        Player* player = static_cast<Player*>(spell->GetUnitTarget());
        player->removeSpell(36122);   // Earthforged Leggings
        player->removeSpell(36129);   // Heavy Earthforged Breastplate
        player->removeSpell(36130);   // Stormforged Hauberk
        player->removeSpell(34533);   // Breastplate of Kings
        player->removeSpell(34529);   // Nether Chain Shirt
        player->removeSpell(34534);   // Bulwark of Kings
        player->removeSpell(36257);   // Bulwark of the Ancient Kings
        player->removeSpell(36256);   // Embrace of the Twisting Nether
        player->removeSpell(34530);   // Twisting Nether Chain Shirt
        player->removeSpell(36124);   // Windforged Leggings
    }
};

// 36436 - Forget                                                               // Unlearn Weaponsmith specialization
struct ForgetWeaponsmith : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        Player* player = static_cast<Player*>(spell->GetUnitTarget());
        player->removeSpell(36125);   // Light Earthforged Blade
        player->removeSpell(36128);   // Light Emberforged Hammer
        player->removeSpell(36126);   // Light Skyforged Axe
        player->removeSpell(36258);   // Blazefury
        player->removeSpell(34537);   // Blazeguard
        player->removeSpell(34535);   // Fireguard
        player->removeSpell(36131);   // Windforged Rapier
        player->removeSpell(36133);   // Stoneforged Claymore
        player->removeSpell(34538);   // Lionheart Blade
        player->removeSpell(34540);   // Lionheart Champion
        player->removeSpell(36259);   // Lionheart Executioner
        player->removeSpell(36260);   // Wicked Edge of the Planes
        player->removeSpell(34562);   // Black Planar Edge
        player->removeSpell(34541);   // The Planar Edge
        player->removeSpell(36134);   // Stormforged Axe
        player->removeSpell(36135);   // Skyforged Great Axe
        player->removeSpell(36261);   // Bloodmoon
        player->removeSpell(34543);   // Lunar Crescent
        player->removeSpell(34544);   // Mooncleaver
        player->removeSpell(36262);   // Dragonstrike
        player->removeSpell(34546);   // Dragonmaw
        player->removeSpell(34545);   // Drakefist Hammer
        player->removeSpell(36136);   // Lavaforged Warhammer
        player->removeSpell(34547);   // Thunder
        player->removeSpell(34567);   // Deep Thunder
        player->removeSpell(36263);   // Stormherald
        player->removeSpell(36137);   // Great Earthforged Hammer
    }
};

// 36438 - Forget                                                               // Unlearn Swordsmith specialization
struct ForgetSwordsmith : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        Player* player = static_cast<Player*>(spell->GetUnitTarget());
        player->removeSpell(36258);   // Blazefury
        player->removeSpell(34537);   // Blazeguard
        player->removeSpell(34535);   // Fireguard
        player->removeSpell(36131);   // Windforged Rapier
        player->removeSpell(36133);   // Stoneforged Claymore
        player->removeSpell(34538);   // Lionheart Blade
        player->removeSpell(34540);   // Lionheart Champion
        player->removeSpell(36259);   // Lionheart Executioner
    }
};

// 36439 - Forget                                                               // Unlearn Axesmith specialization
struct ForgetAxesmith : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        Player* player = static_cast<Player*>(spell->GetUnitTarget());
        player->removeSpell(36260);   // Wicked Edge of the Planes
        player->removeSpell(34542);   // Black Planar Edge
        player->removeSpell(34541);   // The Planar Edge
        player->removeSpell(36134);   // Stormforged Axe
        player->removeSpell(36135);   // Skyforged Great Axe
        player->removeSpell(36261);   // Bloodmoon
        player->removeSpell(34543);   // Lunar Crescent
        player->removeSpell(34544);   // Mooncleaver
    }
};

// 36441 - Forget                                                               // Unlearn Hammersmith specialization
struct ForgetHammersmith : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex effIdx) const override
    {
        if (effIdx != EFFECT_INDEX_0)
            return;

        Player* player = static_cast<Player*>(spell->GetUnitTarget());
        player->removeSpell(36262);   // Dragonstrike
        player->removeSpell(34546);   // Dragonmaw
        player->removeSpell(34545);   // Drakefist Hammer
        player->removeSpell(36136);   // Lavaforged Warhammer
        player->removeSpell(34547);   // Thunder
        player->removeSpell(34548);   // Deep Thunder
        player->removeSpell(36263);   // Stormherald
        player->removeSpell(36137);   // Great Earthforged Hammer
    }
};

struct GameobjectCallForHelpOnUsage : public SpellScript
{
    void OnSuccessfulStart(Spell* spell) const
    {
        UnitList targets;
        MaNGOS::AnyUnfriendlyUnitInObjectRangeCheck check(spell->GetCaster(), 12.f);
        MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyUnitInObjectRangeCheck> searcher(targets, check);
        Cell::VisitAllObjects(spell->GetCaster(), searcher, 12.f);
        for (Unit* attacker : targets)
        {
            if (attacker->IsCreature() && static_cast<Creature*>(attacker)->IsCritter())
                continue;

            if (!spell->GetCaster()->IsEnemy(attacker))
                continue;

            if (attacker->AI())
                attacker->AI()->AttackStart(spell->GetCaster());
        }
    }
};

struct Submerged : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* unitTarget = spell->GetUnitTarget();
        if (!unitTarget)
            return;

        unitTarget->SetStandState(UNIT_STAND_STATE_CUSTOM);
        unitTarget->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNINTERACTIBLE);
    }
};

struct Stand : public SpellScript
{
    void OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* unitTarget = spell->GetUnitTarget();
        if (!unitTarget)
            return;

        unitTarget->SetStandState(UNIT_STAND_STATE_STAND);
        unitTarget->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNINTERACTIBLE);
    }
};

// s.7131 - npc 2638,4785,5097,6493,6932,11027,11263 - might be different delay per npc!
struct IllusionPassive : public AuraScript
{
    SpellAuraProcResult OnProc(Aura* aura, ProcExecutionData& /*procData*/) const override
    {
        if (Unit* caster = aura->GetCaster())
            if (caster->IsCreature())
                static_cast<Creature*>(caster)->ForcedDespawn(1000);
        return SPELL_AURA_PROC_OK;
    }
};

void AddSC_spell_scripts()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "spell_dummy_npc";
    pNewScript->pEffectDummyNPC = &EffectDummyCreature_spell_dummy_npc;
    pNewScript->RegisterSelf();

    RegisterSpellScript<CastFishingNet>("spell_cast_fishing_net");
    RegisterSpellScript<GreaterInvisibilityMob>("spell_greater_invisibility_mob");
    RegisterSpellScript<InebriateRemoval>("spell_inebriate_removal");
    RegisterSpellScript<AstralBite>("spell_astral_bite");
    RegisterSpellScript<FelInfusion>("spell_fel_infusion");
    RegisterSpellScript<AuchenaiPossess>("spell_auchenai_possess");
    RegisterSpellScript<GettingSleepyAura>("spell_getting_sleepy_aura");
    RegisterSpellScript<AllergiesAura>("spell_allergies");
    RegisterSpellScript<UseCorpse>("spell_use_corpse");
    RegisterSpellScript<RaiseDead>("spell_raise_dead");
    RegisterSpellScript<SplitDamage>("spell_split_damage");
    RegisterSpellScript<TKDive>("spell_tk_dive");
    RegisterSpellScript<CurseOfPain>("spell_curse_of_pain");
    RegisterSpellScript<spell_seed_of_corruption_npc>("spell_seed_of_corruption_npc");
    RegisterSpellScript<WondervoltTrap>("spell_wondervolt_trap");
    RegisterSpellScript<ArcaneCloaking>("spell_arcane_cloaking");
    RegisterSpellScript<FoodAnimation>("spell_food_animation");
    RegisterSpellScript<DrinkAnimation>("spell_drink_animation");
    RegisterSpellScript<Drink>("spell_drink");
    RegisterSpellScript<spell_effect_summon_no_follow_movement>("spell_effect_summon_no_follow_movement");
    RegisterSpellScript<SpellHasteHealerTrinket>("spell_spell_haste_healer_trinket");
    RegisterSpellScript<IncreasedHealingDoneDummy>("spell_increased_healing_done_dummy");
    RegisterSpellScript<IncreasedSpellDamageDoneDummy>("spell_increased_spell_damage_done_dummy");
    RegisterSpellScript<spell_scourge_strike>("spell_scourge_strike");
    RegisterSpellScript<TribalDeath>("spell_tribal_death");
    RegisterSpellScript<PreventSpellIfSameAuraOnCaster>("spell_prevent_spell_if_same_aura_on_caster");
    RegisterSpellScript<InstillLordValthalaksSpirit>("spell_instill_lord_valthalaks_spirit");
    RegisterSpellScript<RetaliationCreature>("spell_retaliation_creature");
    RegisterSpellScript<NetGuard>("spell_net_guard");
    RegisterSpellScript<HateToHalf>("spell_hate_to_half");
    RegisterSpellScript<HateToZero>("spell_hate_to_zero");
    RegisterSpellScript<Stoned>("spell_stoned");
    RegisterSpellScript<BirthNoVisualInstantSpawn>("spell_birth_no_visual_instant_spawn");
    RegisterSpellScript<SleepVisualFlavor>("spell_sleep_visual_flavor");
    RegisterSpellScript<CallOfTheFalcon>("spell_call_of_the_falcon");
    RegisterSpellScript<ForgetArmorsmith>("spell_forget_36435");
    RegisterSpellScript<ForgetWeaponsmith>("spell_forget_36436");
    RegisterSpellScript<ForgetSwordsmith>("spell_forget_36438");
    RegisterSpellScript<ForgetAxesmith>("spell_forget_36439");
    RegisterSpellScript<ForgetHammersmith>("spell_forget_36441");
    RegisterSpellScript<MaximizePetLoyalty>("spell_maximize_pet_loyalty_and_happiness");
    RegisterSpellScript<GameobjectCallForHelpOnUsage>("spell_gameobject_call_for_help_on_usage");
    RegisterSpellScript<Submerged>("spell_submerged");
    RegisterSpellScript<Stand>("spell_stand");
    RegisterSpellScript<IllusionPassive>("spell_illusion_passive");
}
