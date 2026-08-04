/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef ALONECRAFT_TEST_LOG_H
#define ALONECRAFT_TEST_LOG_H

#include "Define.h"
#include "StringFormat.h"

#include <string>
#include <string_view>

class Player;
class Unit;

// ---------------------------------------------------------------------------
//  Alonecraft in-game test harness
// ---------------------------------------------------------------------------
//
//  A structured, greppable trace of every Alonecraft script decision, written
//  to its OWN file so it can be read without wading through the server log:
//
//      <LogsDir>/alonecraft_test.log
//
//  Why not the normal Log system: loggers and appenders are read by
//  Log::LoadFromConfig, which Main.cpp calls at line 192 -- long before
//  LoadModulesConfigs at line 263.  A module .conf can therefore never
//  register an appender, so the file is opened directly here.
//
//  Off by default.  `.woatest begin <case>` turns it on, `.woatest end` turns
//  it off, so normal play costs one atomic load per ACTEST site.
//
//  USAGE
//      ACTEST("WARL.RUIN", "spread immolate={} targets={}", spellId, count);
//
//  The format string is Acore::StringFormat (fmt), i.e. {} placeholders.
//  Prefer key=value pairs -- the test plan greps for them.
// ---------------------------------------------------------------------------

namespace Alonecraft::TestLog
{
    /// Cheap enough to call on every hot path; a relaxed atomic load.
    bool Enabled();

    void SetEnabled(bool on);

    /// Appends one timestamped line. Callers normally go through ACTEST.
    void Write(std::string_view tag, std::string const& message);

    /// Absolute path of the trace file, for `.woatest` to report.
    std::string const& FilePath();

    /// Free-form marker line (BEGIN/END/NOTE), always written regardless of
    /// the enabled flag so a session boundary is never lost.
    void Marker(std::string const& message);

    /// Full snapshot of a player: stats, resources, pet, and every aura with
    /// its per-effect amounts. This is what makes the DBC-only changes
    /// verifiable -- they have no C++ hook to instrument.
    void DumpState(Player* player, std::string_view label);

    /// Display name of a unit, or "-" when null. Used to keep log lines short.
    std::string N(Unit const* unit);
}

#define ACTEST(tag, ...)                                                       \
    do                                                                         \
    {                                                                          \
        if (Alonecraft::TestLog::Enabled())                                    \
            Alonecraft::TestLog::Write((tag), Acore::StringFormat(__VA_ARGS__));\
    } while (false)

#endif // ALONECRAFT_TEST_LOG_H
