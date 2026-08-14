/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef ALONECRAFT_SIM_RUNNER_H
#define ALONECRAFT_SIM_RUNNER_H

#include "Define.h"

#include <string>

class Unit;

// ---------------------------------------------------------------------------
//  Alonecraft offline combat simulator
// ---------------------------------------------------------------------------
//
//  Tuning 30 specs by playing them is not viable, so combat is measured instead:
//  a real worldserver boots normally -- real DBC, real spell_proc, all 722
//  alonecraft_spell_dbc overrides and every module script -- but `--sim` swaps
//  the realtime update loop for a fixed-diff one and detaches GameTime from the
//  wall clock.  Every cooldown, GCD, aura duration, swing timer and proc ICD
//  reads that cached clock, so combat stays exactly correct while running as
//  fast as the CPU allows.
//
//  Nothing here reimplements combat maths.  That is the entire point: a
//  standalone model would have to mirror 722 overridden spells and ~26 files of
//  bespoke damage arithmetic, and would drift on every commit.
//
//  MILESTONE 1 SCOPE
//      One bot, one stationary dummy, N virtual seconds, total damage, one JSON
//      file.  No spec file, no iterations, no per-ability breakdown.  Enough to
//      answer "did my change move damage?" without logging in, which is the most
//      acute pain, and to prove the clock works before anything is built on it.
//
//  USAGE
//      worldserver --sim --sim-char <name> --sim-target <creature entry>
//                        [--sim-spec "shadow pve"] [--sim-level 80]
//                        [--sim-seconds 60] [--sim-iterations 1]
//                        [--sim-seed 1] [--sim-out <path>]
//
//      --sim-iterations runs N fights inside one process, reseeding and fully
//      resetting the actor between them.  Booting the world costs ~28 wall
//      seconds against ~1.5 for a 60-virtual-second fight, so a process per
//      iteration would spend almost all of its life loading maps it is about to
//      freeze.  Each fight is reported separately in the result JSON.
//
//      --sim-spec names a playerbots premade spec (AiPlayerbot.PremadeSpecName
//      in playerbots.conf), matched case-insensitively, exact before substring;
//      a bare integer is taken as the index.  Giving it runs the actor through
//      PlayerbotFactory::Randomize, which assigns talents, gear, glyphs,
//      enchants, gems, ammo, consumables and a pet -- the same code path that
//      configures a live random bot.  Omitting it sims the character exactly as
//      stored, which for a fresh sim character means naked.
// ---------------------------------------------------------------------------

namespace Alonecraft::Sim
{
    /// True when --sim was passed, i.e. this process is a simulator run.
    bool Active();

    /// Records damage dealt by the actor (or its pet) to the target.
    void RecordDamage(Unit* attacker, Unit* victim, uint32 amount);

    /// Records healing done by the actor (or its pet). `gain` is the effective
    /// heal; the overheal is recovered by pairing it with the latched
    /// pre-clipping amount from ModifyHealReceived.
    void RecordHeal(Unit* healer, Unit* receiver, uint32 gain);

    /// Timestamps the death of the actor, the target, or the actor's pet.
    void RecordDeath(Unit* unit);
}

#endif // ALONECRAFT_SIM_RUNNER_H
