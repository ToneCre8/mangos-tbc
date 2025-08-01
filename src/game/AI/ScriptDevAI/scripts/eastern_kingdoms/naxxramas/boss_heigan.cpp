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
SDName: Boss_Heigan
SD%Complete: 80
SDComment: Missing poison inside the eye stalk tunnel in phase 2
           Candidate spell is 30122 (correct damage range and already used in encounter) but there is no evidence of this
           and of how the spell is cast and by who (no data found in sniffs as this part was removed in WotLK)
SDCategory: Naxxramas
EndScriptData */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "AI/ScriptDevAI/base/BossAI.h"
#include "naxxramas.h"

enum
{
    PHASE_GROUND            = 1,
    PHASE_PLATFORM          = 2,

    SAY_AGGRO1              = 13041,
    SAY_AGGRO2              = 13042,
    SAY_AGGRO3              = 13043,
    SAY_SLAY                = 13045,
    SAY_TAUNT1              = 13046,
    SAY_TAUNT2              = 13047,
    SAY_TAUNT3              = 13048,
    SAY_TAUNT4              = 13050,
    SAY_CHANNELING          = 13049,
    SAY_DEATH               = 13044,

    // Heigan spells
    SPELL_DECREPIT_FEVER    = 29998,
    SPELL_MANA_BURN         = 29310,
    SPELL_TELEPORT_SELF     = 30211,
    SPELL_TELEPORT_PLAYERS  = 29273,
    SPELL_PLAGUE_CLOUD      = 29350,                // Channel spell periodically triggering spell 30122
    SPELL_PLAGUE_WAVE_SLOW  = 29351,                // Activates the traps during phase 1; triggers spell 30116, 30117, 30118, 30119 each 10 secs
    SPELL_PLAGUE_WAVE_FAST  = 30114,                // Activates the traps during phase 2; triggers spell 30116, 30117, 30118, 30119 each 3 secs
    SPELL_TELEPORT_TRIGGER  = 29499,

    MAX_PLAYERS_TELEPORT    = 3,

    NPC_PLAGUE_WAVE         = 17293,                // Control plague waves
    NPC_WORLD_TRIGGER       = 15384
};

static const float resetX = 2825.0f;                // Beyond this X-line, Heigan is outside his room and should reset (leashing)


enum HeiganActions
{
    HEIGAN_TAUNT,
    HEIGAN_ERUPTIONS,
    HEIGAN_PHASE_PLATFORM,
    HEIGAN_PHASE_GROUND,
    HEIGAN_CHANNELING,
    HEIGAN_ACTION_MAX,
    HEIGAN_DOOR,
};

struct boss_heiganAI : public BossAI
{
    boss_heiganAI(Creature* creature) : BossAI(creature, HEIGAN_ACTION_MAX), m_instance(static_cast<ScriptedInstance*>(creature->GetInstanceData()))
    {
        m_creature->GetCombatManager().SetLeashingCheck([&](Unit*, float x, float /*y*/, float /*z*/)
        {
            float respawnX, respawnY, respawnZ;
            m_creature->GetRespawnCoord(respawnX, respawnY, respawnZ);
            return m_creature->GetDistance2d(respawnX, respawnY) > 90.f || x > resetX;
        });
        SetDataType(TYPE_HEIGAN);
        AddOnKillText(SAY_SLAY);
        AddOnDeathText(SAY_DEATH);
        AddOnAggroText(SAY_AGGRO1, SAY_AGGRO2, SAY_AGGRO3);
        AddCombatAction(HEIGAN_PHASE_PLATFORM, 90s);
        AddCombatAction(HEIGAN_TAUNT, 25u * IN_MILLISECONDS, 90u * IN_MILLISECONDS);
        AddCustomAction(HEIGAN_ERUPTIONS, true, [&]() { StartEruptions(m_phase == PHASE_GROUND ? SPELL_PLAGUE_WAVE_SLOW : SPELL_PLAGUE_WAVE_FAST); });
        AddCustomAction(HEIGAN_PHASE_GROUND, true, [&]() { HandleGroundPhase(); }, TIMER_COMBAT_COMBAT);
        AddCustomAction(HEIGAN_DOOR, true, [&]() { CloseEntrance(); });
        AddCustomAction(HEIGAN_CHANNELING, true, [&]() { HandleChanneling(); }, TIMER_COMBAT_COMBAT);
    }

    ScriptedInstance* m_instance;

    uint8 m_phase;

    void Reset() override
    {
        CombatAI::Reset();

        m_phase = PHASE_GROUND;

        StopEruptions();
    }

    uint32 GetSubsequentActionTimer(uint32 id)
    {
        switch (id)
        {
            case HEIGAN_TAUNT: return urand(25, 90) * IN_MILLISECONDS;
            default: return 0; // never occurs but for compiler
        }
    }

    void Aggro(Unit* unit) override
    {
        BossAI::Aggro(unit);
        ResetTimer(HEIGAN_ERUPTIONS, 5u * IN_MILLISECONDS);
        ResetTimer(HEIGAN_DOOR, 15u * IN_MILLISECONDS);
    }

    void JustDied(Unit* killer) override
    {
        BossAI::JustDied(killer);
        StopEruptions();
    }

    void EnterEvadeMode() override
    {
        ScriptedAI::EnterEvadeMode();

        if (m_instance)
            m_instance->SetData(TYPE_HEIGAN, FAIL);

        StopEruptions();
    }

    void HandleGroundPhase()
    {
        for (uint32 action : {HEIGAN_TAUNT})
            ResetCombatAction(action, 5000);
        AddInitialCooldowns();
        m_creature->InterruptNonMeleeSpells(true);
        SetCombatMovement(true, true);
        SetMeleeEnabled(true);
        StopEruptions();
        m_phase = PHASE_GROUND;
        ResetTimer(HEIGAN_ERUPTIONS, 100u);
        ResetCombatAction(HEIGAN_PHASE_PLATFORM, 90u * IN_MILLISECONDS);
    }

    void HandlePlatformPhase()
    {
        for (uint32 action : {HEIGAN_TAUNT})
            DisableCombatAction(action);
        SetCombatMovement(false, true);
        SetMeleeEnabled(false);
        if (DoCastSpellIfCan(nullptr, SPELL_TELEPORT_SELF) == CAST_OK)
        {
            StopEruptions();
            m_phase = PHASE_PLATFORM;
            ResetTimer(HEIGAN_ERUPTIONS, 3000u); // Small delay to let enough time for players to reach the first safe zone
            ResetTimer(HEIGAN_CHANNELING, 100u);
        }
        ResetTimer(HEIGAN_PHASE_GROUND, 45u * IN_MILLISECONDS);
    }

    void StartEruptions(uint32 spellId)
    {
        // Clear current plague waves controller spell before applying the new one
        if (Creature* trigger = GetClosestCreatureWithEntry(m_creature, NPC_PLAGUE_WAVE, 100.0f))
        {
            trigger->RemoveAllAuras();
            trigger->CastSpell(trigger, spellId, TRIGGERED_OLD_TRIGGERED);
        }
    }

    void StopEruptions()
    {
        // Reset Plague Waves
        if (Creature* trigger = GetClosestCreatureWithEntry(m_creature, NPC_PLAGUE_WAVE, 100.0f))
            trigger->RemoveAllAuras();
    }

    void CloseEntrance()
    {
        if (!m_instance)
            return;

        if (GameObject* door = m_instance->GetSingleGameObjectFromStorage(GO_PLAG_HEIG_ENTRY_DOOR))
            door->SetGoState(GO_STATE_READY);
    }

    void HandleChanneling()
    {
        DoBroadcastText(SAY_CHANNELING, m_creature);
        DoCastSpellIfCan(nullptr, SPELL_PLAGUE_CLOUD);
        // ToDo: fill the tunnel with poison - required further research
    }

    void ExecuteAction(uint32 action) override
    {
        if (!m_instance)
            return;

        switch (action)
        {
            case HEIGAN_TAUNT:
            {
                switch (urand(0, 3))
                {
                    case 0: DoBroadcastText(SAY_TAUNT1, m_creature); break;
                    case 1: DoBroadcastText(SAY_TAUNT2, m_creature); break;
                    case 2: DoBroadcastText(SAY_TAUNT3, m_creature); break;
                    case 3: DoBroadcastText(SAY_TAUNT4, m_creature); break;
                }
                ResetCombatAction(action, GetSubsequentActionTimer(action));
                return;
            }
            case HEIGAN_PHASE_PLATFORM:
            {
                HandlePlatformPhase();
                DisableCombatAction(action);
                return;
            }
            default:
                return;
        }
    }
};

/*###################
## npc_diseased_maggot
####################*/

struct npc_diseased_maggotAI : public ScriptedAI
{
    npc_diseased_maggotAI(Creature* creature) : ScriptedAI(creature)
    {
        m_instance = (instance_naxxramas*)creature->GetInstanceData();
        Reset();
    }

    instance_naxxramas* m_instance;

    uint32 m_resetCheckTimer;

    void Reset() override
    {
        m_resetCheckTimer = 3 * IN_MILLISECONDS;
    }

    void UpdateAI(const uint32 diff) override
    {
        // Do nothing if no target
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_resetCheckTimer <= diff)
        {
            // Check if we are in range of the trigger NPC in the middle of Heigan room, if so: force evade
            if (Creature* trigger = GetClosestCreatureWithEntry(m_creature, NPC_WORLD_TRIGGER, 45.0f))
                m_creature->AI()->EnterEvadeMode();
            m_resetCheckTimer = 3 * IN_MILLISECONDS;
        }
        else
            m_resetCheckTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

// 29351 - Plague Wave Controller (Slow)
// 30114 - Plague Wave Controller (Fast)
struct PlagueWaveController : public AuraScript
{
    void OnPeriodicTrigger(Aura* aura, PeriodicTriggerData& /* data */) const override
    {
        Unit* triggerTarget = aura->GetTriggerTarget();
        uint32 spellForTick[6] = { 30116, 30117, 30118, 30119, 30118, 30117 };  // Circling back and forth through the 4 plague areas
        uint32 tick = (aura->GetAuraTicks() - 1) % 6;

        triggerTarget->CastSpell(triggerTarget, spellForTick[tick], TRIGGERED_OLD_TRIGGERED, nullptr, aura, aura->GetCasterGuid(), nullptr);
    }
};

// 29499 - Teleport Trigger
struct TeleportTrigger : public SpellScript
{
    void OnInit(Spell* spell) const override
    {
        spell->SetMaxAffectedTargets(MAX_PLAYERS_TELEPORT);
    }

    void OnEffectExecute(Spell* spell, SpellEffectIndex /*effIdx*/) const override
    {
        Unit* target = spell->GetUnitTarget();
        if (target)
            target->CastSpell(target, SPELL_TELEPORT_PLAYERS, TRIGGERED_OLD_TRIGGERED);
    }
};

void AddSC_boss_heigan()
{

    Script* newScript = new Script;
    newScript->Name = "boss_heigan";
    newScript->GetAI = &GetNewAIInstance<boss_heiganAI>;
    newScript->RegisterSelf();

    newScript = new Script;
    newScript->Name = "npc_diseased_maggot";
    newScript->GetAI = &GetNewAIInstance<npc_diseased_maggotAI>;
    newScript->RegisterSelf();

    RegisterSpellScript<PlagueWaveController>("spell_plague_wave_controller");
    RegisterSpellScript<TeleportTrigger>("spell_teleport_trigger");
}
