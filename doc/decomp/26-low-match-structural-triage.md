# Low-match structural triage (2026-09-26)

Baseline: `main` commit `1b3725b5f4f329724a0117b3fd910a31bf570840`.
Both before and after reports use the local reference SHA-256
`30d3fe55503416139a1d0f9b25fc73924aab4b12dd731aafc08b4a1c9bd950d5`.
The baseline was regenerated from the unmodified linked library, not assumed
from the committed report. This pass targets fuzzy function matching, not
byte-for-byte ELF layout, GOT ordering, or data placement.

## Retained changes

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `NetworkObjectManager::SendPushMessage` | 0% | 74.93% |
| `NetPredictor::AllowPush` | 0% | 57.09% |
| `AdjustLayerBits` | 0% | 45.80% |

- `ReplicatorData` has four words, not three. Retail `AllowPush` copies offsets
  `+0`, `+4`, `+8`, and `+0xc` at `0x52e620`–`0x52e637`. Preserve the otherwise
  unused final word and assert its target layout.
- `AllowPush` returns `i32`, not `bool`: its virtual caller tests EAX at
  `0x532847`. The base and all overrides now agree. Its two cursor allocations
  are separately aligned, and the allowed result shares the timestamp update.
- `SendPushMessage` uses a pointer loop over the eight peer entries. The former
  integer-index loop was fully unrolled at the existing `-O3`, producing 3,127
  bytes versus retail's 810. The pointer loop produces 770 bytes without
  changing the optimization setting.
- `AdjustLayerBits` uses the signed layer indices with their `-1` sentinel,
  and branches to OR the Geonosian layer into the existing mask. Retail
  sign-extends both indices and does not emit the reconstructed extra `& 31`.

Overall fuzzy matching: **62.948578% → 62.977623%**. Six function scores
improve and one regresses; no full matches are lost. The ABI-correct fourth
word changes local stack layout in `CalcReplicatorDataSize`, whose score
changes from 69.32% to 69.21%. Do not shrink the structure to recover that
small score difference.

## Rejected or deferred experiments

- Moving the unchanged `NuSoundWeakPtr<T>::Set` body outside its class restores
  the retail call in `NuSoundDecoder::RequestBuffer` (`0x31fb62`), improving
  that function from 0% to 46.06% in the linked library. However, it also
  regresses multiple streamer functions and slightly reduces the aggregate
  score. The shared-header change was reverted. A future attempt must check
  all weak-pointer callers, not just `RequestBuffer`.
- `ShoveSystemCheckGameObject` remains 0% with its assigned `-O1` after a
  count-cache/control-flow experiment. An isolated diagnostic compilation
  of unchanged source at `-O2` reaches 82.47872% against the relocatable object.
  This is evidence to audit translation-unit/optimization provenance, not
  permission to change the map or add a function optimization attribute.
  The experiment and configuration change were not retained.
- `LevelComplete_LSW_Draw` is a 274-byte retail wrapper with a compiler-generated
  `.part.27` callee. The reconstruction is a 2,467-byte unsplit function.
  Active-first and nested control-flow rewrites and a temporary caller probe
  did not reproduce splitting. All were reverted; do not hand-author a
  register-ABI clone or repeatedly rewrite this body without new evidence.

## Verification

The target build, all five `//scripts/checks:checks` tests, symbol-surface
check, and `git diff --check` pass. The symbol check reports no missing
required symbols and no extra-symbol baseline drift. `matching.json` and
the README badge were regenerated from the final linked library. No native,
WASM, or gameplay execution was performed in this pass.

## Arcade reconstruction batch

Baseline: `5dd0ad35`. The nine remaining arcade stubs now implement the
retail scoring, kill counters, coin thresholds, save progression, panel, and
end-menu behavior. Definitions follow the original function order. The
mobile `Arcade_BothPlayersActive` really is a constant true function; do not
replace it with a speculative player-presence check.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `Arcade_AwardPoint` | 5.12% | 100% |
| `Arcade_PlayerKilled` | 11.35% | 100% |
| `Arcade_Kill` | 15.00% | 100% |
| `Arcade_DrawEndMenu` | 10.50% | 100% |
| `Arcade_UpdatePanel` | 9.55% | 99.93% |
| `Arcade_DrawPanel` | 6.67% | 99.89% |
| `Arcade_UpdateEndMenu` | 11.05% | 99.92% |
| `Arcade_AIKilled` | 4.88% | 99.28% |
| `Arcade_CoinCollected` | 4.47% | 99.35% |
| `Blowup_Activate` | 0.13% | 17.28% |

Structural findings:

- The per-player kill arrays occupy `AreaGlobals + 0x24` and `+0x2c`.
  Named array aliases preserve the old field names and layout assertions.
- `Arcade_Score` is a separate two-word array from `Arcade_Points`.
- The per-level mode mask is an eight-bit value. Keeping it as a `u32`
  unnecessarily widens the test and introduces another saved register.
- Player/AI/coin entry points reject unsigned player indices greater than
  one. AI and coin thresholds use unsigned comparisons.
- Three near-full panel/menu scores differ only in local data or literal
  addresses. The scorer still penalizes these slightly. Do not spend a
  source-reconstruction pass trying to fix them by moving constants or
  changing the scoring metric.
- The opponent expression `(player_index + 1) & 1` retains the retail mask
  but emits `add` where retail uses `xor`. Equivalent XOR/complement forms
  let GCC remove the mask and score slightly lower. No optimizer override
  or instruction-forcing workaround was retained.

`Blowup_Activate` now groups activation-only work and updates its flag fields
directly. Two adjacent functions change only the scratch registers in one
load/test pair each: `Blowup_SetVisibility` 99.57% → 99.24%, and
`Blowup_AddGizmos` 87.93% → 87.58%. The complete batch remains net-positive:
**62.977623% → 63.022400%**, with four new full fuzzy matches and none lost.

Further unsuccessful experiments were reverted:

- Debris collision Y-expression grouping, one-based loop traversal, ordered
  lifetime gates, and cached sample times did not recover the retail loop
  shape. A cached-time variant reached only 5.72% against an object file.
- `refpack` explicit hash initialization, zero-length copy guards, and hash
  update placement still scored 0%. Revisit only with new structural evidence.
- Equivalent branch/arithmetic forms of `Player_HasDoubleBoltDamage_FromBolt`
  and `InstantKillParts` generated the same instructions.

Verification: target compilation and the complete linked report succeeded.
A focused 32-bit harness linked the actual NDK-compiled arcade object and
passed assertions for invalid player indices, disabled arcade, score/reset
gating, all three mode-save bits, completion/autosave, AI thresholds, coin
mode precedence and resets, null players, end-menu timing, and draw gating.
This is not a full gameplay run. The temporary harness is not a canonical
repository test target.

## Credits, icon animation, and particle scaling

Baseline: `33650891`. Five more stubs are reconstructed without changing
their source files' optimization settings.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `Credits_Load` | 0.87% | 39.10% |
| `Credits_DrawPanel` | 3.00% | 99.91% |
| `Credits_UpdateMenu` | 2.31% | 81.00% |
| `UpdateIconWibble` | 3.09% | 99.71% |
| `CreateScaledPARTEffect` | 2.98% | 58.41% |

Overall fuzzy matching: **63.022400% → 63.094010%**. Eight scores improve,
none regress, and no full matches are lost. Reintroducing the writable
credits duration also improves `Credits_GetInfo` from 92.73% to 99.95%.

- Credits entries and styles are both 24 bytes. The loader owns at most
  1,000 entries, reads `stuff\\text\\english_credits.txt`, accepts the first
  positive duration override while the duration is still its 120-second
  default, and maintains per-style colour overrides. Title/name entries
  share a row. The completion panel and skip/audio/fade state machine are
  reconstructed from the same original TU.
- The four icon angular velocities are a previously missing private `i32`
  array. Each icon updates its own timer, consumes two random values on
  expiry, and wraps its angle through an integer conversion. A normal
  inline helper called for the four channels reproduces retail's structure;
  a rolled loop scored only 23.77% against the object.
- Particle scaling follows the parent type stored at `+0x174`, reuses the
  closest active relative scale within retail's **1.1** tolerance, and clones
  a free type otherwise. The source type's name is shortened to 12 characters
  before its three-digit suffix. Slot zero is populated but its allocation
  still returns the previous closest ID, exactly as retail does. The input
  check accepts 128 despite the 128-element table; normal loaded IDs remain
  below 128. This pre-existing binary edge case was not silently changed.
- `Credits_DrawPanel` and `UpdateIconWibble` have the original instruction
  structure; their remaining score differences are data/literal addresses.
  Explicitly splitting the loader's initialization/search/colour and style
  cases did not improve its score. The simpler, better-scoring form was kept.

The target build and full linked report pass. A temporary 32-bit test harness
compiled the same sources with the NDK and unchanged optimization settings;
function/data sections were enabled only in the test objects so unrelated
engine functions could be discarded at link time. Tests pass for credits
loading, duration/colour overrides, paired rows, draw clipping, the 1,000-entry
cap, allocation/parser failure, completion rendering, skip/audio/fade timing,
icon random-call order and angle wrapping, and particle reuse/allocation.
These are focused tests with mocked external services, not a full gameplay run.

Further structural triage: retail `NuFrameEndBgLoadPS` and
`UCStretchToCorners` exhibit unoptimized frame/temporary patterns while their
current owners compile at `-O2`; no source churn or optimization override was
attempted. `Push_UpdateHints` has eight unrolled retail player checks versus
a rolled current loop. Resolve these provenance questions before spending
another speculative matching pass on them.

## Challenge rewards and asteroid levels

Baseline: `4ce3464d`. Eight low-matching stubs are reconstructed, with the
existing optimization settings unchanged.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `ChallangeCash_Update` | 2.49% | 70.88% |
| `AsteroidChaseB_Init` | 4.00% | 99.49% |
| `AsteroidChaseB_Draw` | 16.15% | 99.92% |
| `AsteroidChaseB_Update` | 2.75% | 91.38% |
| `AsteroidChaseC_Init` | 3.13% | 99.73% |
| `AsteroidChaseC_Update` | 2.91% | 90.68% |
| `AsteroidChaseD_Init` | 0.84% | 93.63% |
| `AsteroidChaseD_Update` | 0.86% | 99.78% |

Overall fuzzy matching: **63.094010% → 63.2454%**. Eleven scores improve,
one decreases slightly, and no full matches are lost. `Asteroids_Reset`
changes from 80.80% to 80.75%; a direct before/after disassembly comparison
shows only local-data/literal references moving, not changed instructions or
control flow.

The useful structural correction in this batch is source ownership:
`DrawFalconSpotLights` and its four private timer arrays were stranded in
`render.cpp`, despite being used by the asteroid B initialization/update/draw
group. They now live together in `episodeV.cpp`. Both effective Bazel compile
actions use `-O3` and the same target options. Removing the now-unnecessary
`__used__` retention attribute from this genuinely called static helper lets
GCC infer retail's register argument automatically. No calling-convention
attribute is introduced. The helper itself improves 97.87% → 99.93%, and its
draw caller improves from an initial reconstruction at 90.92% to 99.92%.
The helper body and its pre-existing local matrix alignment are unchanged.

Other recovered contracts:

- The final asteroid is a 60-byte record: one special, eight blowup pointers,
  a signed 16-bit count, three signed rotation speeds, and three signed
  rotations. Its network packet is eight bytes. Initialization caps target
  collection at eight and replaces near-zero random speeds; clients seek the
  host angles while the host advances and publishes them.
- Asteroid B collects up to eight classic blowups but awards pickups for the
  first four, once per destruction/reappearance cycle. Falcon lights wait
  until the frame after the timer reaches two seconds and likewise clear
  one frame after fading to zero. The signed 32-bit area-mask shift is kept
  as observed rather than silently widened to a different 64-bit operation.
- Asteroid D retains an old turret pointer when its named gizmo is missing,
  but replaces it when a gizmo is found, even if the object is null. Fourteen
  of the sixteen turrets receive escape-ship targets; indices 8 and 9 are
  intentionally skipped. Existing targets are not overwritten. Destruction
  decrements the counter once, clients do not advance the completion timer,
  and story/free-play routes differ after twelve seconds.
- Challenge rewards emit 50 coin messages only when crossing one second,
  use three random calls per message, and stagger delays by 0.1 seconds.
  The empty-queue check at 1.5 seconds precedes the burst check, so a large
  initial timestep can skip the burst. Stage completion uses strict `>`.

No attempt was made to force missing stack realignment in the challenge or
asteroid C update functions. Asteroid D's initializer uses a single clear of
the 96-byte escape array; retail redundantly clears pieces of that storage.
These residual shapes are recorded instead of adding score-only attributes
or redundant clearing solely for an instruction match.

The target build and full linked report pass. A temporary focused 32-bit
harness links the NDK-compiled source objects, with function/data sections
enabled only for test dead stripping. Assertions pass for the 50-message
reward burst, random-call order, message fields, queue/timing boundaries,
stage advancement, asteroid B pickup/reset and spotlight draw/fade gates,
asteroid C target cap/reinitialization, host/client rotation and translation
preservation, and asteroid D missing/null turrets, target mapping, cutscene
fire interval, one-time destruction, and both completion destinations.
External services are mocked; this is not a full gameplay run.

## Hoth wave controller and background creatures

Baseline: `eef5a5a4`, after rebasing onto main's cutscene-flag correction
(`2bdd275d`). Both remaining Hoth controller stubs are reconstructed at the
existing `-O3` setting.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `HothBattleE_UpdateWave` | 0.59% | 82.54% |
| `HothBattle_ManageBackgroundCreatures` | 0.99% | 71.58% |
| `HothBattleE_Init` | 98.74% | 99.84% |

Overall fuzzy matching: **63.2454% → 63.3283%**. Three scores improve and
two decrease slightly; no full matches are lost. Direct before/after
disassembly shows only moved data/literal references in
`SpawnMeleeCreatureType` (56.79% → 56.77%), and those plus one scratch-register
load/test change in `AsteroidChaseC_Init` (99.73% → 99.62%). Their behavior
and control flow are unchanged.

Recovered contracts:

- The global melee timer is a float at `+8`, not the first word of wave
  zero. Four 40-byte wave records start at `+0xc`; six background-creature
  pointers start at `+0xac`. The complete record remains 200 bytes. Layout
  assertions preserve all established accesses, while named fields expose
  the transition state and initial, remaining, and active counts.
- Wave changes wait five seconds except for the first wave, select normal
  or low-end camera scripts, wait for the cutscene to begin and end, and
  retry wave startup until it succeeds. Final-wave completion clears the
  record's tail and routes story/free-play differently, then continues the
  camera-state setup as retail does.
- Cleanup tests character **model flags at `+4`**, not the separate flags
  at `+0x40`, and excludes AT-ATs and player objects. Dead wave objects and
  those with a nonzero owner-index byte are removed and counted once, with
  the original pitch cue.
- Wave three creates two antinodes using the completed spawn-loop index
  for both radii. The signed 32-bit exclusion-mask expression is retained.
  Do not replace either with a superficially more natural per-object value.
- Background spawning caps probes/AT-STs at 5/6 normally and 2/2 on low-end
  devices, suppresses creature types already present in the wave, and
  assigns each successful spawn to its locator. Locator lookup still occurs
  before the disabled/transition gates. Failed probe creation breaks into
  cleanup/AT-ST handling; a missing locator returns immediately.
- Reverse background cleanup preserves retail's handling of null holes;
  it is not a generic compact-all-nonnull operation.

Two bounded source-form experiments were retained: declaring the AT-ST
limit before the probe limit and expressing the loop comparisons in retail
operand order raised the background object comparison from 50.66% to
71.16%; separating the positive transition phase from the cutscene gate
raised the controller from 81.45% to 82.11%. Linked scores are slightly
higher. Remaining differences are predominantly branch placement, stack
slots, and register allocation; no optimizer or calling-convention
attributes were added.

The target build and full linked report pass. Focused 32-bit tests link the
actual NDK-compiled source with external services mocked. Assertions cover
all transition phases and delay boundaries, normal/low-end scripts, missing
scripts, object cleanup and one-time removal, count repair, antinode radii
and masks, both final destinations, normal/low-end spawn limits, locator
assignment, null holes, and locator/object allocation failures. These are
not a full gameplay run. Function/data sections are enabled only in the
temporary test object for dead stripping, not in matching builds.

## Hoth panel, shared level allocation, and bonus pickups

Baseline: `4ea624d4`. Four further stubs are reconstructed without changing
optimization settings.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `HothBattleE_Panel` | 0.88% | 78.59% |
| `ChrisAllocLevelStuff` | 6.27% | 99.97% |
| `LegoCity_Update` | 0.98% | 57.66% |
| `NewTown_Update` | 1.36% | 59.51% |

Overall fuzzy matching: **63.328278% → 63.422913%**. Six scores improve and
four decrease slightly; no full matches are lost. The three Cloud City
handlers (`CloudCityTrapA_Update`, `CloudCityTrapB_Update`,
`CloudCityTrapC_Panel`) and `DeathStar2BattleD_InZapRange` have only changed
scratch-register load/test pairs and data/literal references in a direct
before/after comparison. No behavior or control flow changes in those bodies.

The allocator exposes an important missing contract: dogfight state reserves
`0x63ef4` bytes, not the formerly described `0x62ef0` prefix. Its last known
word at `+0x62ef0` is cleared; a further 4 KiB remains opaque. The recovered
float at `+0x62ee4` is normalized speed, initialized to one and replaced by
socket speed divided by eleven when nonzero. The 256-record reset arena is
inside the allocation at `+0x5ce90`. No whole-block clear is added.

The same allocator reserves a `0xaf24`-byte pod-race record for the three
pod-race levels. `PODRACE_s` and its existing pointer-bearing lap records now
live in a shared header so both allocation and initialization use the same
`sizeof`, including on 64-bit hosts. The source definitions and target layout
are unchanged. The world flag at `+0x511c` is named and asserted. Other levels
clear that flag but leave the existing data pointer alone, as retail does.

Hoth's panel uses local 16-entry target/defeated arrays and a persistent
16-float alpha array. Waves one/two insert a row break after half-plus-one
targets; wave three has one row; wave four places its AT-AT first, then probes,
a separator, and AT-STs. Defeated targets seek alpha 0.4 by steps of 0.1;
other entries reset to one. A mini cut suppresses all drawing. Clients forward
the existing 88-byte packet's 12-entry arrays and count directly; the retail
panel does not publish a host packet, so no inferred publication was added.

The city/town updates scan all eight player slots for live vehicle objects
with owner-index byte zero whose linked rider is in context `0x3b`. Occupancy is stored
as **0/255**, not boolean 0/1. Only an occupancy change updates pickup groups,
and pickups with runtime bit 8 are excluded. Occupancy is remembered even if
the pickup array is null or empty; a missing world/system returns before
changing that history. Lego City maps tractor/tauntaun/mooncar/towncar to
groups 3/4/5/6; New Town maps tauntaun/firetruck/lifeboat to 2/3/4.

Focused tests pass against the actual NDK objects for all Hoth panel rows,
alpha transitions, cutscene suppression, client forwarding, allocation sizes
and fields, zero/negative/NaN socket speeds, and all city/town player slots,
eligibility gates, duplicate riders, entry/exit, absent pickup arrays, and
2,000 randomized occupancy transitions. The allocator tests also pass with
a 64-bit host object: space/pod-race allocations grow to 409344/45096 bytes
from the target's 409332/44836 bytes. External services are mocked, and this
is not a full gameplay run. The full target and native builds also pass.

Additional bounded experiments were not retained: swapping GEONOSIAN fall
animation tests did not recover its branch layout; expanding the eight rumble
checks through an ordinary inline helper still had different cold-block
placement (36.20%, 815 bytes versus retail 707). A ternary occupancy-change
expression slightly helped one bonus level but hurt the other; the simpler
XOR form is retained. These are not reasons to change optimization settings.

## Signal occupancy, suit exchange, and Death Star lightning

Baseline: `9cf24397`. Three further stubs are reconstructed at their existing
`-O3` setting.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `Signals_Update` | 0.92% | 95.30% |
| `Signal_MoveCode` | 1.87% | 33.40% |
| `DeathStar2BattleD_Update` | 1.77% | 93.08% |

Overall fuzzy matching: **63.422913% → 63.505257%**. Ten scores improve,
two decrease, one reaches 100%, and no full matches are lost.

- Signals consume one random value even while inactive. Active unused signals
  copy `AddGameMsg_Default`, override the text/position/colour/flags fields,
  and fade toward one. Used signals scan all eight players and retain their
  in-use bit only for the matching character inside both geometric bounds;
  their scale fades toward zero. Inactive signals clear the bit and scale
  immediately. The squared proximity constant is `0x3efae14a`, reproduced by
  `(0.6f + 0.1f) * (0.6f + 0.1f)`, not rounded to `0.49f`. Both comparisons
  are strict and reject NaNs. The ordinary eight-element loop naturally
  unrolls with the existing compiler settings.
- Signal movement owns context `0x4c` and animation `0x96`. Missing animation
  data advances the timer; present-but-not-playing animation data pauses it.
  Exchange happens at the animation marker or completion, swaps the stored
  suit pointers, replaces capability bits, and records the new suit among
  the ten area suit bits. The old exchange bit is captured before
  `StartEndOfJump`; the subsequent feedback test reads the possibly updated
  bit. No inferred character reload or sound effect is added.
- `GameObject + 0x788` gains a typed signal-pointer alias while retaining the
  opaque alias and its offset assertion. This layout-neutral union changes
  GCC alias analysis in existing consumers: `Grapple_LookAtPos` gains its
  retail reloads and reaches 100%, and `GizGetBuildItPlayerPos` rises from
  76.42% to 93.20%. Direct before/after inspection shows the two decreases
  are one extra pointer reload in `MechTouchTaskPullLever::Update`
  (62.44% → 61.60%) and two reordered independent loads/stores in
  `Grapple_DrawLine` (54.44% → 54.41%), plus relocated data references.
- Death Star lightning traverses player pointers without unrolling, enables
  and positions each light halfway to the target, draws the beam, applies
  feedback and disorientation, and preserves all four random draws per
  affected player, including the otherwise unused first draw. A destroyed
  inner shield suppresses that traversal and can emit a pickup on integer
  timer transitions into multiples of three. Pickup direction uses cosine
  for X and sine for Z; random pickup radius is 10–25. Global rumble is a
  separate optional prelude and does not suppress either path.

Bounded experiments: moving the signal fade-target initialization emitted
identical code. Alternate suit-exchange branch/goto layouts scored worse;
the straightforward conditional form is retained. The remaining suit score
is principally a basic-block-placement problem, not a reason to add compiler
attributes. Named Death Star temporaries and explicit colour assignments
slightly improved its object comparison; its emitted size equals retail's
1,136 bytes.

Target and native builds pass. Focused tests link the NDK-compiled functions
with external services mocked and also pass against 64-bit host objects.
Coverage includes all eight signal/player slots, message-default preservation,
strict geometry and NaN boundaries, 2,000 randomized occupancy frames, every
signal-entry gate, animation/timer transitions, all ten suits and an external
suit, capability replacement, single exchange, and feedback. Death Star tests
cover all players, light/beam vectors, random consumption and threshold
boundaries, missing/inactive shields, timer crossings (including negative
times), pickup parameters, and the global-rumble prelude. The previous
2,000-frame city/town pickup regression also passes. Tests use function/data
sections only in temporary test objects; matching builds are unchanged. No
full gameplay run was performed.

## Batarang flight and duplicate placeholder ownership

Baseline: `06f6edb7`. The Batarang changes retain the source's `-O3` setting.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `Batarang_SeekToTarget` | 0.64% | 93.27% |
| `Batarang_InitRicochet` | 6.63% | 85.60% |
| `Batarang_Ricochet` | 68.39% | 99.97% |
| `seed_chase` | 2.61% | 100% |
| `_fseek64_wrap` | 20.00% | 100% |
| `ParseAIPathCnxFlag` | 7.50% | 86.05% |

Overall fuzzy matching: **63.505257% → 63.560116%**. Six scores improve,
none decrease, two reach 100%, and no full matches are lost. The reference
code denominator remains 4,722,419 bytes and 13,459 function records.

Flight now uses the existing `Batarang_GetTargetPos`, moved unchanged from
`bolts.cpp` to its actual caller's `batarang.cpp`, instead of a simplified
substitute. The original local entry point remains emitted using its existing
retention annotation: retail also calls it from an unreconstructed target-marker
path in `Batarang_MoveCode`. Removing retention with only the current caller
inlines it and loses the required symbol. Its remaining private-register-ABI
difference is deferred until that real second caller is recovered; no calling
convention attribute or artificial call is added.

The seek path distinguishes a lost/out-of-range target from helper failure,
uses the owner's joint when returning in flight, accelerates steering after
one second, and ray-tests only before four seconds. Target type and platform
identity determine whether an impact starts a ricochet. Non-ricochet flight
decays the signed ricochet count, integrates velocity, and tests a strict
0.25-unit arrival radius. The ricochet timer tests its value at entry before
advancing, rather than expiring one frame early. Initialization rotates and
combines normalized vectors, retains 80% speed, and seeks each component by
ten; the old simplified 75%-speed reflection was not retail behavior.

Five unused nine-byte local placeholders duplicated real linked functions:

- `episode.cpp::seed_chase` and `legoapi_misc.cpp::_fseek64_wrap` duplicated
  the pinned libvorbis implementations in `psy.c` and `vorbisfile.c`.
- `edpath.cpp::ParseAIPathCnxFlag` duplicated the parser in `aisys.cpp`.
- `render/fx/edsplines.cpp::SplineLength` duplicated the local helper in
  `socksysall.cpp` (the separate public editor function is retained).
- `parts.cpp::UpdateAnimTimer` duplicated the helper used by `apiobject.cpp`.

Each exact local symbol occurs once in retail. Source searches found no calls
to the placeholders; the real bodies and their callers are unchanged. Removing
only the placeholders leaves one emitted symbol apiece and removes three
ambiguous source assignments. The library functions were already 100% matches;
name-only pairing had selected the placeholders. Library ownership is outside
the report's game-source unit list, so these two entries become unassigned
rather than being incorrectly attributed to game stubs. The spline and timer
scores are unchanged. This extends the duplicate trap documented in the audio
audit: inspect all same-name local symbols before reconstructing a stub. A
follow-up scan found no other short/large duplicate pair with a unique retail
symbol except translation-unit startup functions, which were left alone.

Target and native builds and all five repository tests pass. Focused tests
link the actual NDK-compiled Batarang source and also pass with a 64-bit host
object. Coverage includes all target kinds, owner/joint fallback, helper
failure, platform filtering, steering/ray-test timing boundaries, strict
arrival distance, signed count decay, timer NaNs, ricochet limits, and 1,000
randomized rotation/speed samples. External math/terrain services are mocked;
this is not a full gameplay run. Function/data sections are enabled only in
temporary test objects, not matching builds.

## Rancor exclusions and Sarlacc disco display

Baseline: `1f09ba25`. Both reconstructed handlers retain Episode VI's `-O3`.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `JabbasPalaceE_Update` | 0.93% | 75.97% |
| `SarlaccPitB_SpecialUpdate` | 2.22% | 99.85% |
| `SarlaccPitB_Reset` | 87.18% | 92.04% |

Overall fuzzy matching: **63.560116% → 63.639454%**. Five scores improve,
four decrease slightly, and no full matches are lost. Direct before/after
inspection attributes the decreases to literal relocation, a short/long branch
encoding and alignment fill in `EmperorFightA_Reset`, scratch-register choices
and independent load/store scheduling in `SarlaccPitC_Reset`, and scratch
registers in `SarlaccPitC_Update`. `SetLevelSfxBits` changes only literal address
operands. Their behavior and control-flow destinations are unchanged.

The Rancor handler performs host-only boss completion/level routing, then
rebuilds the boss's 64-bit opponent-exclusion mask. A live creature set excludes
all present players; otherwise the three safe areas decide exclusion. The
retail area expression is a **signed 32-bit shift widened to 64 bits**, not a
64-bit area shift: bit 31 sign-extends. The reconstructed expression explicitly
masks the shift count and preserves that widening on both pointer widths.
Player identities use the separate full 64-bit mask. Two proximity explosions
each play their sound once on activation and reset their latch when inactive
or absent. A combined loop condition scored 50.69% in the object comparison;
separate creature-set branches recover the original unrolled paths and reach
75.88% without optimizer attributes.

Sarlacc's private `sarlaccdisco` now has a typed 1,024-byte target layout:
five 16-element special arrays, area/obstacle/message pointers, count, height,
and still-opaque scalar fields. Initializer/reset accesses use typed members
and `sizeof`, so host pointers and special records cannot overlap their
neighbors through hardcoded 32-bit offsets. Target offset assertions preserve
the retail contract. The 20-byte network packet gains its height, six signed
panel masks, active byte, and 16-bit sound latch. The recovered data global
`disco_base_offset` is initialized to retail's `-0.12f`.

Display hides all five variants before selecting the first set mask in
off/flash/select/on/finish order. Clients seek the height toward the packet
**once per panel**, while the host publishes its height once per panel. The
base special follows that height plus the offset, and the sound mask uses the
on-special's matrix translation. Active disco updates its completion message,
three radios, mirror-ball obstacle, floor/light visibility, shutter, and paired
doors; the completion sound is latched until disco becomes inactive. Missing
position/animation pointers are handled exactly where retail checks them.

A shared selected-special variable merged the five display paths and scored
25.58%. An ordinary inline helper called from each original branch preserves
the separate call sites, reproduces the retail 1,580-byte body, and scores
98.98% before linking. No helper symbol or forced inlining is added. The typed
initializer keeps its original instruction shape and score. Reloading the
signed count after each reset iteration now improves the typed reset to 92.04%;
the earlier byte-array experiment in the Episode VI notes had not done so.

Target/native builds and all five repository tests pass. Focused 32-bit NDK
and 64-bit host tests cover boss routing, every player/area slot, signed area
masks, explosion sound edges, and 2,000 randomized exclusion frames. Sarlacc
tests cover initialization for every count 0–16, typed pointer assignments,
reset visibility, all five display priorities at all 16 panel bits, host/client
height updates, missing positions, completion sound reset/replay, and missing
special/animation gates. The 64-bit Sarlacc test also passes AddressSanitizer
and UBSan. The previous Death Star lightning regression passes against the
final NDK object. External services are mocked; no full gameplay run is claimed.

A bounded `Push_UpdateHints` experiment was not retained: eight explicit calls
to an ordinary inline predicate still differ in prologue and branch layout at
the source's required `-O2` (0% versus the existing 0.50%). Changing optimization
settings is not a permitted shortcut.

## Sarlacc puzzle progression and Geonosian fall timer

Baseline: `63f13577`. Episode VI remains `-O3`; `gameanim.cpp` remains `-O2`.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `SarlaccPitB_Update` | 0.39% | 51.34% |
| `Animate_GEONOSIAN` | 0.60% | 54.32% |

Overall fuzzy matching: **63.639454% → 63.696384%**, with four improved
scores, one small decrease, and no full matches lost. The decreased
`DeathStar2BattleD_Init` score (99.91% → 99.86%) is an equivalent exchange of
two stack spill slots, plus literal references; direct before/after inspection
shows no changed behavior. `SarlaccPitC_Update` and `SetLevelSfxBits` improve
incidentally. The matching denominator is unchanged.

The previously opaque Sarlacc fields at `+0x3cc` are sixteen byte panel states;
`+0x3dd` is the phase, the next two signed bytes are selected panel indices,
and `+0x3e0` is a floating timer. `+0x3fc` is a real position pointer, now
pointer-width-aware on hosts. Reset's four-word clear becomes an equivalent
fixed-size `memset` without changing its score. Recovered retail data globals
are `sarlaccdiscotime = 40`, `discoheightseek = 2`, and `discoheight = 1.27`.

Host progression clears sound edges, refreshes three AI messages, queries the
special panel, and requires two completed build-its plus animation-set state
2. Entering the disco area selects two distinct off panels. Both occupied
panels latch on and select the next pair; exhausting the available panels
enters completion and fills the panel states with the finish variant. The
active packet byte changes on the following frame, as in retail. Every 2.5
seconds without simultaneous occupancy, up to two lit panels revert to off.
The first qualifying player in each eight-slot scan determines occupancy and
whether the human-controlled character should request the companion's help.
The distance test is strictly below `0.2f * 0.2f`, using object position rather
than collision position, and requires both `0x1001` object flags and contact.

The area bit uses the same signed-32-bit widening as the Rancor handler. Host
updates publish five masks and ease the floor toward the target height; clients
skip those decisions. Both paths animate the visible engine lump and call the
shared display update. Losing readiness during the puzzle resets and continues
through mask/display publication. Expiry or lost readiness during completed
disco resets and returns immediately, leaving the old panel masks untouched
for that frame. The initial no-panel/one-panel case retains retail's `-1`
indices instead of silently adding a new completion transition.

The first source reconstruction is 4,793 bytes versus retail's 4,747. Tests of
an occupancy variable in the loop condition and of ending the loop by changing
its index both produce 4,729 bytes but slightly worse object matching
(50.86% versus 50.93%); neither is retained. Remaining differences are mainly
block order, scratch allocation, and loop shape. No optimizer/calling-convention
attributes or hand-written assembly were introduced.

Geonosian's timer now selects a floating result and performs one final store,
instead of distinct add/store and integer-zero-store source paths. Its
animation predicates and call order are unchanged. Reordering the three
high-jump animation alternatives gives no useful improvement, so their original
source order is retained. The local helper's compiler-inferred register ABI
remains intact.

Target/native builds and all five repository tests pass. The actual NDK-built
Sarlacc test covers all eight player slots, every init/display count and mask,
4,096 area-index/area-mask combinations, exact distance/time thresholds,
sound edges including bit 15, readiness/reset paths, companion hints, height
clamps, completion timer NaNs, network-client preservation, and engine motion.
It also checks 2,000 randomized occupancy frames. Geonosian tests exhaust all
65,536 signed animation IDs with both high-jump flag states and seven timer
values, including negative, NaN, and infinity, plus animation changes made by
the idle callback. Both test suites pass on 64-bit hosts with AddressSanitizer
and UBSan. External services are mocked; no full gameplay run is claimed.

## Death Star fire rendering and progression

Baseline: `473319dc`; Episode VI remains `-O3`.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `DeathStar2BattleFire_Draw` | 1.60% | 94.22% |
| `DeathStar2BattleFire_Update` | 0.42% | 99.41% |

Overall fuzzy matching: **63.696384% → 63.824688%**, with four improved scores,
one 0.0019-point decrease, and no full matches lost. The tiny
`SetLevelSfxBits` decrease consists entirely of literal-address operands in
`lea` instructions. The two other changed drawing scores improve incidentally.

Draw recovers three independent passes over a local `SPLINEPOS_s`: backward
from the fire front, forward from the rear to the front, then backward from the
rear. Each pass steps 40 world units even when its particles are clipped. The
clip global is **signed integer** `fire_clip_dist = 1000`, converted to float
for a strict squared-distance comparison; it is not a floating configuration
value. The point-along query supplies angles, while particle position comes
from the local spline record. Pitch and yaw narrow to signed 16-bit arguments.
The pauses, matrix initialization calls, and global spline preservation follow
retail. The first source version has the exact 1,680-byte retail size; remaining
differences mainly schedule independent call arguments differently.

Update recovers `fireDeltaPos`, computed from player zero's run speed or a
30-unit fallback, and preserves both random generators' consumption order.
It updates the fire cube, seeds the rear spline when required, then checks all
eight non-excluded player slots in fire-local coordinates. Negative longitudinal
distance emits particles at the **nearest player found so far**, but damage is
applied to the current slot. Controlled-player contact slows the fire. The
hurt-sound suppression flag is saved before emission and restored after a hit;
the impact-sound flag follows retail's one-shot hit behavior. The recovered
0.2/1/1.75 speed decisions include gradual recovery without clamping its final
step. Front movement has the level/startup-timer gate; rear movement is always
backward at the unscaled base speed. Optional explosion audio precedes the
continuous lantern sound. The first source version is 4,557 bytes versus
retail's 4,549, with no matching-only attributes or assembly.

Target/native builds and all five repository tests pass. Focused NDK and
sanitized 64-bit host tests cover all three render passes, exact and NaN clip
boundaries, integer-to-float rounding, signed angles, pause behavior, unchanged
global splines, all player slots, nearest-emitter/current-victim separation,
ignored damage, suppression-flag restoration, speed boundaries/recovery,
startup/level movement gates, spline endpoints and NaNs, random/effect/sound
boundaries, and 2,000 randomized player frames. External services are mocked;
no full gameplay run is claimed.

## Character loading, nearest detonators, and player rumble

Baseline: `447c5d9c`. Character core remains `-O3`; detonators and gamepads
remain `-O2`.

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `UpdateCharacterLoad` | 0.00% | 29.77% |
| `Detonator_FindNearest` | 0.98% | 30.37% |
| `NewRumbleAllPlayers` | 0.95% | 36.56% |

Overall fuzzy matching: **63.824688% → 63.847305%**. Only these three scores
change; none regress and no full matches are lost.

The character loader's integer-index store-pack loop expanded into eleven
copies, making its body 1,698 bytes versus retail's 868. A typed pointer loop
with a separate pack index restores traversal and reduces it to 962 bytes.
The separate index avoids pointer-distance division before each store query.
Fixed-character priority, low-end gates, callback reloads, capacity checks,
collection filtering, and the random candidate calculation are unchanged.
An alternate shared selection label and a value-terminated integer scan were
tested separately and not retained. Remaining block/register differences are
not a reason to change this file's optimization mode.

`Detonator_FindNearest` returns a **`DETONATOR_s *`**, not `void`. Its previously
opaque first twelve bytes are the logical `NUVEC position`; the render position
at `+0xc` is not used for this search. The reconstructed query considers ten
active slots in order, with separate owner-filtered and unrestricted paths.
Zero radius uses a finite squared-distance limit of `1e9`; other radii are
squared, including negative values. Strict comparisons preserve the first
equal-distance entry and reject NaN distances. Ordinary inline calls recover
the fixed-slot structure without adding a helper symbol or forced-inlining
attribute. A compact loop is behaviorally correct but reaches only 4.03% in
the object comparison; fixed-slot calls reach 30.18% (30.37% linked). The
remaining differences include stack storage and branch scheduling.

Player rumble likewise has eight explicit retail slot checks. Ordinary inline
calls preserve null/controller-flag/pad gates, per-call integer conversion,
duration selection, and the reload of later player slots after sound callbacks.
The scaled floating amount is calculated once, as in retail. An early-return
helper form produces the same score as its nested equivalent. No optimization
or calling-convention attributes were added.

`Prompt_LSW_Update` was investigated but left unchanged. Retail indexes the
two activity bytes, tests the adjacent challenge/mission bytes together, and
uses a shared post-scan selection path. Typed aliases alone leave its object
score at 0%; bounded shared-loop variants reach only 10.37–14.59% while still
changing frame and branch structure substantially. All prompt/header
experiments were removed. Do not repeat them without new evidence about the
remaining control-flow or compiler provenance.

Target/native builds and all five repository checks pass. Focused NDK and
sanitized 64-bit host tests cover character-loading gates and priorities,
all store packs, callback mutations, and 5,000 randomized selections; every
detonator activity mask, all owners/slots, strict radius and tie behavior,
NaNs/infinities, unchanged source records, and 10,000 randomized queries;
and every rumble slot mask, controller/pad gates, callback mutations,
duration/rate boundaries and NaNs, and 5,000 randomized cases. Sound/loading
services and the distance callback are mocked; no gameplay run is claimed.

## Customiser accessories and Anakin door state

The next batch raises linked fuzzy matching from **63.847305% to 63.918064%**.
Four functions improve, three scores regress, and no 100% matches are lost.
Both source files retain `-O2`; `chris.cpp` also retains its existing
`-fno-ipa-sra`. No source ownership or calling-convention changes are involved.

| Function | Before | After |
|---|---:|---:|
| `Customiser_DumpAccessories` | 0.72% | 97.16% |
| `Customiser_DrawAccessories` | 1.95% | 24.21% |
| `ChrisAnakinCReset` | 1.36% | 76.78% |
| `ChrisAnakinCUpdate` | 5.16% | 75.58% |

Accessory cleanup uses the existing typed `Accessory[2][9]` resource array.
The loaded flag must equal one, and only model-slot `-1` suppresses a side.
Each named category either removes its scene and clears the scene pointer
after the callback, or restores the original material texture, updates that
material, and destroys the reloaded texture ID. Retail does not clear texture
IDs or the loaded flag, and scene ownership takes precedence over texture
ownership. Eighteen ordinary inline calls recover the fixed-slot structure;
there is no forced inlining or additional emitted helper.

Accessory drawing selects the helmet locator and character side once, then
checks positive piece counts, category exclusions, helmet-layer flags, and
special existence. Each matrix is copied after the existence callback;
reflection state is read after the primary draw. The two matrix locals use
the existing aligned type because retail explicitly realigns their stack.
A post-increment resource cursor improves the object score from 18.50% to
24.07%; the linked result is 24.21%. The remaining frame, saved-piece
selection, and loop-scheduling differences are not solved by adding artificial
padding or changing optimization.

`AnakinC` was incorrectly typed as a `GameObject_s` pointer. Reset/update
establish an array of twelve `0x140` door records, with four matrices, two
special handles, translation state, activity, and a platform ID. The shared
header now expresses that pointer-width-aware layout with target assertions.
The existing volatile pointer and its unallocation behavior are preserved.
`DoorSetupList` is recovered as fifteen `0x28` records; all string pairs and
scalar payloads were compared against the linked retail table at `0x624b80`.
Its last primary name is an empty string, not NULL. The reconstruction keeps
that data and the retail caller contract: allocated storage and twelve
successful door matches, or a null-name terminator supplied by the caller.
It does not invent allocation or a new safety termination condition.

Reset compacts successful lookups into those twelve slots. A failed secondary
lookup leaves the old secondary flag untouched; a successful lookup narrows
the existence result to 16 bits. The original/current matrices use chained
assignment, matching the retail copy order. Returning directly from the
null-name cleanup path raises object matching from 22.71% to 76.53%; changing
the primary lookup from early-continue to nested form makes no difference.
Update preserves inactive records, clamps ordered offsets to their minimum,
transforms both door halves, and reloads callback-visible state. Explicit
stores in the two clamp branches and a three-float position copy improve its
object score from 7.60% to 75.15%, avoiding a compiler-generated SIMD select
without changing floating-point semantics.

`Customiser_AddPartAccessories` is unchanged in source but falls 0.66 points;
before/after disassembly shows register allocation, instruction scheduling,
and literal placement changes with unchanged operations. The `TrueHero` and
`MiniKit` draw scores move by -0.0024 and -0.0017 points respectively from
relocated references only. All three were reviewed; no gameplay correction
was needed in those neighboring functions.

Target/native builds and all five repository checks pass. Focused NDK and
ASan/UBSan 64-bit host tests pass for both subsystems. Accessory tests cover
all category masks, ownership combinations, character sides, model-slot
sentinels, 16 locators, unsigned saved-piece indices, reflection order,
callback mutations, and 5,000 randomized draw cases. Door tests cover
lookup/secondary failures, compaction and inactive tails, preserved fields,
narrowing, all 4,096 activity masks, NaNs/infinities/signed zero, callback
reloads, and 3,000 randomized reset/update cases. Rendering and scene services
are mocked; no gameplay execution is claimed.

## Flight-spline evaluation and XZ intersection

The spline batch raises linked fuzzy matching from **63.918064% to
63.952187%**. Four functions improve, one caller score regresses slightly,
and no 100% matches are lost. `render/fx/edsplines.cpp` keeps its existing
`-O3`; no source moves, optimization overrides, or calling-convention
attributes are introduced.

| Function | Before | After |
|---|---:|---:|
| `CalcSplinePoint` | 1.73% | 53.31% |
| `CalcSplinePointFromDist` | 8.89% | 99.98% |
| `BezierLineEval` | 6.27% | 54.97% |
| `EvaluateSplineXZIntersection` | 1.76% | 78.17% |

`flightspline_s` was empty even though pod creation already accessed its
fields through raw byte offsets. `FlightSpline_Init` establishes a `0x52c`
stride: 64 four-float points, count at `0x400`, total distance at `0x410`,
64 cumulative distances at `0x414`, and ID at `0x524`. The shared pod header
now expresses those verified fields and keeps the remaining bytes opaque.
The first `0xa580` bytes of `PODRACE_s` become 32 such records without moving
the lap entries or changing the allocation size. Pod creation and startup
use this shared type. The loader itself remains a stub.

Point evaluation constructs normalized, ten-unit endpoint tangents, applies
the retail cubic expression to XYZ, and linearly interpolates W. Its fraction
is computed before the index clamp; the upper clamp is the point count, not
count minus two. This retains the retail requirement for valid neighboring
point storage rather than adding different endpoint or invalid-count behavior.
Count and point data are reloaded after normalization callbacks where retail
does so. Endpoint and normalization temporaries preserve their documented
16-byte stack alignment. Snapshotting the outer control points and expressing
the component operations directly improves object matching from 46.33% to
52.45%; individual endpoint stores reach 53.21%. Remaining temporary-lifetime,
frame, and register-allocation differences are left documented rather than
forcing spills or altering compiler flags.

Distance conversion uses the first cumulative distance strictly above the
query, divides its interpolated index by the point count, and forwards one
for distances at least the total length. If no interval matches, it forwards
the unchanged query. Its linked score is 99.98%; the object-level differences
are relocation references. Bézier evaluation preserves the four Bernstein
weights, float operation grouping, input/output aliasing, and zero W.

The XZ intersection query clears both output records before assigning their
spline pointers and narrowed loop flags. Scan bounds use the full-width loop
arguments. The first spline supplies `logical_count - 1` segments; the second
supplies `logical_count`, including its closing segment even when not looping.
Only strictly closer ordered distances replace the result; a zero-distance
hit exits the inner scan, not the outer scan. Finalization measures each
selected segment, scales its stored fraction, and calls `MoveSplinePosition`
with `0.00001f`. Point stride is the retail hardcoded three-float size, not
the spline's `pt_size` field. Conditional count expressions and shared
fraction-local scope raise object matching from 73.95% to 78.03%.

The typed pod caller changes from 36.43% to 36.29%. Before/after disassembly
shows equivalent pointer arithmetic, an earlier load of spline length, and
relocated constants; its function size remains 2,344 bytes. No behavior
correction or optimization change was made to that caller.

Target/native builds and all five repository checks pass. Focused NDK and
ASan/UBSan 64-bit host harnesses pass for both spline subsystems. Evaluation
tests cover counts 2 through 61, segment/fraction boundaries, output aliasing,
normalization callback mutations, distance fallback, signed zero and
non-finite coordinates, Bézier aliases/non-finite parameters, and 5,000
randomized cases. Intersection tests cover counts -2 through 8, seven loop
flag values, null/empty inputs, aliased outputs, strict ties and non-finite
distances, wraparound, callback-modified counts/point arrays/fractions,
finalization order, and 5,000 randomized cases. Geometry services are mocked;
no gameplay execution or invalid-storage support is claimed.

## WEIRDO animation dispatch

Recovering `Animate_WEIRDO` raises its linked match from **1.60% to 58.80%**
and aggregate fuzzy matching from **63.952187% to 63.965984%**. No other
function scores change. The file retains `-O2`, and the three shared private
animation helpers retain their existing local symbols and compiler-inferred
calling conventions. This removes the last `STUBBED()` body in `gameanim.cpp`;
the old comment promising restoration of local linkage after four missing
callers was stale and has been corrected.

The dispatcher preserves context-owned animation, ground/fall gates, jump
priority, forced weapon idle, movement and idle variants, extra-action remaps,
blend checking, and the final idle callback. Fall timing uses the animation
selected after that callback. Saber sound is restricted to weapon IDs 101,
103, 105, and 107; the imperial guard is excluded, the bodyguard has a separate
loop, and the remaining sound depends on the dark-side model flag. Sound
lookup is a separate statement before reading playback volume, matching the
retail callback ordering. That correction improves object matching from
54.31% to 58.52% and prevents an early volume snapshot. Separate lookup calls
inside each sound branch score 57.27%, while explicit fall-path labels emit
the same 58.52% code as the retained structured expression; both experiments
were removed.

Target/native builds and all five repository checks pass. NDK and ASan/UBSan
64-bit harnesses pass for every signed 16-bit weapon ID and animation ID,
both high-jump states, all signed 8-bit contexts with 256 weapon-idle flag
combinations, callback mutations, non-finite movement/timer values, sound
ordering, and 10,000 randomized cases. The test oracle follows the retail
dispatcher and reuses the existing private helpers to test their integration;
it does not independently re-validate those helpers. External animation and
sound services are mocked; no gameplay run is claimed.

### Deferred facing-push-block reconstruction

`NearestFacingPushBlock` is another structural ABI issue: the retail function
returns a `pushblock_s *`, while the current stub returns `void`. Retail gates
on `LEGOCONTEXT_PUSH`, tests cardinal facing windows, scene visibility,
vertical/lateral bounds and packed direction flags, then picks the strictly
nearest candidate inside a squared-distance limit. Two provisional bodies
compiled under the unchanged `pushblocks.cpp -O1` setting produce 1,360 and
1,334 bytes versus 1,474 retail bytes and both score 0%. Capturing the bounds
does not resolve the very different branch layout, folded angle ranges,
duplicated early-return stores, or `0x40` versus `0x50` frame. Both code probes
were removed before integration and are not behavior-validated. Investigate
source/compiler provenance before repeating these trials; do not change the
optimization setting or add matching-only attributes to force a score.

## Detonator movement and placement

Recovering `Detonator_MoveCode` raises its linked match from **0.71% to
37.05%**. Correcting `ThermalDetonator_MoveCode` raises it from **2.02% to
32.18%**. Together they raise aggregate fuzzy matching from **63.965984% to
63.996925%**.
This removes the last stub in `detonator.cpp`, retaining its `-O2` setting,
existing record layouts, and the canonical animation/antinode service ABIs.
The only other report change is **-0.0019 points** for unchanged
`SetLevelSfxBits`: its before/after disassembly has the same instruction
sequence and size, with shifted string-relative operands. No exact matches
are lost.

The recovered state machine distinguishes tap-to-place/pick-up from
hold-to-detonate, including the signed input latch and context gates. Holding
selects the strictly oldest active record across all owners, starting with a
finite `-1.0f` age limit. Both placement scans stop at three owned records;
their first available slot may be an active record belonging to another
object, not just an inactive one. Pickup preparation leaves destination Y
untouched. An unfinished pickup whose animation timer expires deliberately
falls through to placement after clearing its context. The animation-frame
gates, antinode registration/removal, callback-visible target and rotation
reloads, and attachment sound follow the retail order.

Three bounded source-shape trials were measured: compact slot loops score
31.52% in the object comparison; short-circuit slot visitors score 37.26%
but emit an extra out-of-line helper call; separating the slot visit from
its count test scores 36.81% and restores fully inlined scans. The latter is
retained for its retail call structure (37.05% after linking). Remaining
differences include push/pop versus frame-slot register saves, frame size,
branch placement, and frame-pointer lifetimes. No matching-only attributes
or optimization changes were used; defer further tuning until new evidence
explains those structural differences.

Target/native builds and all five repository checks pass. NDK 32-bit and
ASan/UBSan 64-bit harnesses pass for all signed context values, all input
latch bytes with finite/non-finite timer boundaries, all **59,049**
inactive/owned/other-owned slot patterns, **10,000** randomized oldest-record
selections, animation frame/timer/marker edge combinations, pickup and
placement failures, and service callback mutations. These tests exercise the
existing nearest-query and detonation helpers as part of the integration;
external geometry, animation, audio, and antinode services are mocked. No
retail gameplay run is claimed.

The thermal routine previously used a hand-written particle scan with an
invented draw-callback filter. Retail instead calls `FindPart(NULL, 0,
object)`, now declared in its owning `parts.h`. The entry resource gate is
slot **0xe9**, not the thrown model at **0xea**, and does not apply to an
already-running throw animation. Entry preserves the movement request bit,
resets the animation only when `AnimPlaying` succeeds, and clears the context
completion bit after that callback. An unavailable frame freezes the timer;
expiry requests the throw if not already completed; the no-clip event uses
strictly less than half a second. Existing requests propagate to the context
completion bit even when no new frame event fires. Young active particles
also block repeat detonation for unordered ages, following the retail
comparison. One reconstruction trial scores 31.91% before linking; further
branch-layout tuning is deferred.

The thermal harness additionally covers all signed contexts, all particle
activity bytes, all weapon-state bytes and animation variants, resource-slot
selection, no-frame freeze, completed/requested flag combinations,
finite/non-finite age/frame/marker/timer boundaries, optional reset, and
callback changes to context, model, animation, flags, and frame time. Both
NDK and sanitized 64-bit runs pass, with particle lookup and other engine
services mocked.

### Additional low-score structural triage

`DisplaySceneRndrSpecials` has an existing body but its current
`render_stubs.cpp` compile command has no optimization option (therefore
`-O0`), while the retail code shows optimized register/branch behavior.
Resolve source ownership and compiler provenance before trying local
expression changes. No source or build option was changed.

`CollideBoltStarFighter` has a different blocker: its retail private ABI
passes the bolt and fighter in EAX/EDX and returns an integer, while the
current local stub returns `void` and its callers remain unreconstructed.
The fighter type is also empty. Recover the shared fighter layout and real
callers together so GCC can infer the private convention; do not add a
calling-convention attribute to the isolated stub. This pass inspected the
retail body but made no speculative source changes to that cluster.

## Random character selection and balloon release

Recovering `RandomIDFromFlags` raises its linked match from **4.27% to
60.75%**, and `LetGoOfBalloon` rises from **3.93% to 75.86%**. Aggregate
fuzzy matching rises from **63.996925% to 64.009445%**. These remove the last
stub bodies in `charconfig.cpp` and `carrying.cpp`, respectively; their
`-O2` and `-O3` settings are unchanged. No other function scores change and
no exact matches are lost.

Random selection filters model-list exclusions, signed customiser IDs,
default movement, required model/game flags, packed visibility flags,
collection ownership, and optional hat support in retail callback order.
It stores at most 500 signed 16-bit IDs, but continues invoking filters for
later characters. Zero and one candidate do not consume random state;
larger lists use `qrand() / (65535 / count + 1)`, not modulo. The character
array pointer is captured before traversal while the count is reloaded
after callbacks. The existing hat helper is reused without modification.

The first source placement slightly changed register allocation in three
unchanged neighbors, including an exact category lookup. One bounded
definition-order trial places random selection after the category helpers,
consistent with their retail address ordering. This restores all three
neighbor scores without changing random selection's 429-byte body or its
60.41% object score. Ownership, compiler options, and calling conventions
remain unchanged; no artificial register or inlining controls are used.

Balloon release clears context before checking the level resource, resolves
the signed hand locator, constructs its translation matrix and rotated
velocity, and copies particle defaults after the rotation callback. Radius
lookup precedes reloading the resource pointer and particle fields. The
new particle receives the low 16 bits of the object's hit flags only after
successful allocation. One body trial scores 75.35% before linking;
remaining differences include the retail aligned frame and register/stack
layout, which are not forced with matching-only attributes.

Target/native builds and all five repository checks pass. Focused 32-bit
NDK and 64-bit host ASan/UBSan tests pass for both functions. Random selection
tests cover all 32 required-mask bits, packed flag bytes, callback ordering
and mutations, the 500-entry cap, signed ID narrowing, all 65,536 random
values at six representative counts, and 1,000 randomized cases. Its host
harness disables ASan global instrumentation (`-mllvm -asan-globals=0`) so
unused parser callback tables can be garbage-collected; stack/heap ASan and
UBSan remain enabled. Balloon tests retain default ASan instrumentation and
cover all signed contexts, all 16 hand locators, byte flags, allocation
outcomes, default snapshots, and callback changes to resources and frame
time. Engine services are mocked; no gameplay execution is claimed.

### Rejected shared sound-list experiment

`NuSoundSystem::CreateVoice` remains a shared-representation issue. Its
retail intrusive-list conversions preserve null before applying an offset;
the current `NuEListOffset::GetLinks` applies the offset unconditionally.
A null-preserving helper raises this function from 0.74% to 15.22%, but the
full report regresses overall: 14 functions improve and seven regress,
including clock removal/destruction, sample unloading, and voice stopping.
Inverting the decoder-selection branches scores 0%, both with and without
the helper change. All experiments were reverted, and no behavior-validation
claim is made for them. Audit the shared intrusive-list representation and
its callers together before revisiting this cluster; isolated source-shape
churn or optimizing only one caller is not a useful next step.

## Minikit collection, completion burst, and message ABI

Recovering `CollectMinikit` raises its linked match from **3.04% to 68.65%**;
`AddStatusMiniKitParts` rises from **0.77% to 73.41%**. Aggregate fuzzy
matching rises from **64.009445% to 64.062720%**, retaining the source's
`-O3` setting. Each body was compiled in one source-shape trial (67.93% and
72.41% before linking). No exact matches are lost. The unchanged
`CharMiniKit_Draw` improves by 1.06 points through register allocation;
`MiniKit_LSW_Draw` loses 0.03 points through four register-operand changes.
Both neighbors retain their previous sizes; their remaining before/after
differences are relocated operands. These are not behavior changes.

Collection requires resource slot **0xce**. It captures the save-slot index
from `WORLD->level_sub_id`, constructs a model message with panel target,
animation/end callbacks, and starts the panel display timer even if message
allocation fails. Only a valid save-slot index with fewer than ten saved
pieces records a name and narrowed level ID, increments the transient
counts, and emits demo hints at counts one and ten. Retail accepts names of
length eight, including the terminator temporarily overlapping the level
field before that field is written; longer names become empty. The
transient ten-entry array's valid-capacity precondition is preserved.
Debris and pickup sound follow regardless of saved progress. Callback-time
world/count changes are reloaded at the same points as retail.

The completion burst snapshots three panel light colours/directions and
ambient colour into the newly recovered, canonical **0x144-byte**
`rtldata_s KitPartRTL`. Every enabled saved piece emits a particle and five
coin messages, consuming exactly seventeen random values. The coin delay
accumulates by 0.1 seconds across pieces, and only the first coin emits
rumble and debris. Each message chooses its player after copying defaults,
then reloads the current coin scale, resource, score, and duration. Panel Y
and lighting are entry snapshots. Resource IDs retain signed 16-bit
interpretation, allocation failures do not stop the sequence, and panel
lights are restored even when no pieces are enabled. Existing `KitPart`
ownership/linkage is not changed and no aligned-stack attribute is added.

### Shared callback and special-handle representation

The new collection callback exposed a structural bug in `ADDGAMEMSG`:
offset **0x3c** is a per-tick function pointer, not a float. Its existing
offset-0x40 callback is instead invoked during drawing. Retail
`UpdateGameMessages` calls the stored tick pointer at runtime offset
**0x104**, while the previous reconstruction passed it through a `u32`,
truncating pointers on 64-bit hosts. Both delay and tick callbacks now have
function-pointer types in the public, queue, and renderer layouts.

Model messages also store a three-pointer special handle, previously copied
as three floats. The shared union and queue copy now preserve that handle's
full width. All existing `extra_position` callers were checked: they pass
level-object special handles, not position vectors. Target offsets/sizes
are asserted, and native-independent size/callback-offset assertions keep
the queue and rendering views consistent. These shared repairs change no
Android function scores; the target queue's instruction sequence is
unchanged apart from relocated operands.

Target/native builds and all five repository checks pass. Focused NDK
32-bit and ASan/UBSan 64-bit tests pass for all resource/save bytes, ten
transient slots, name-length boundaries, signed level narrowing, demo-count
transitions, allocation outcomes, and service callback mutations. Completion
tests cover all **1,024** enabled masks with eight mutation modes, random
endpoints, five-message delay order, snapshot timing, and signed model IDs.
The real queue tests cover full-width handle/callback round trips, all 128
slots, delayed/tick/end dispatch, text/null-special cases, and rejected
specials. A further integration harness executes the reconstructed
collection, real queue insertion, minikit animation tick, completion, and
expiry together. External engine services remain mocked; no gameplay run
or complete host-rendering audit is claimed.

### Deferred minikit counter completion

`IncrementMinikitCounter` needs additional behavioral evidence before
reconstruction. Its retail completion paths pass the local vector at
`esp+0x58` to `AddGameMsgCount`, but the initial `Players_AveragePos` call
fills `esp+0x4c` and no write to the later vector appears in the function.
The camera offset uses a third vector at `esp+0x64`. Do not silently replace
the uninitialized-position path with an assumed intended position, or add
an uninitialized C++ read merely to imitate its stack shape. This pass
inspected the full body and existing average-position helper; it leaves
the stub unchanged pending caller/retail-runtime evidence.

## Versioned flight-spline loading

`FlightSpline_Init` is reconstructed at the existing `-O3`, with its ordinary
C++ linkage and signature unchanged. The linked match rises from **0.80% to
84.71%**, taking the aggregate from **64.062720% to 64.117065%**. The report
has nine improvements, no regressions, and no lost exact matches; the other
eight improvements are below 0.004 percentage points each.

The full retail body at `0x2389d0` establishes these distinct stages:

- Append the mutable `FSP_Extension` (`".FSP"`) to the supplied world's
  config path, select editor-file media 1, and leave all destination data
  untouched if opening fails.
- Read all spline records before computing or loading distances. Promote
  the recovered float/integer fields at `0x408`, `0x40c`, `0x514`, `0x51c`,
  and `0x520` into the canonical `flightspline_s`; retain unknown bytes at
  `0x404` and `0x518`. The structure remains `0x52c` bytes.
- Versions through 1 default the second float and first trailing integer
  to zero; versions through 2 default the later pair to `-1` and the
  record index. All versions read four floats per point and set `0x528`
  to one.
- Versions through 3 integrate backward from parameter 1 to 0, clamping
  the last step. The step is `PODRACE_SPLINEINC` only when the current
  global world's area matches non-null `PODRACE_ADATA`; otherwise it is
  `0.01f`. These globals are re-read between math callbacks.
- Version 4 consumes the serialized length but replaces it with ten
  samples per point interval, storing accumulated lengths at `0x414`.
  The literals are independently rounded `sample / 10.0f`, not repeated
  additions of `0.1f`. Later versions read the length and table directly.
- Clear only `point_count` in remaining destination slots, then close the
  file. The capacity argument is not a file-count clamp, and a nonpositive
  file count starts the clearing pass at zero. Retail trusts file/storage
  sizes and valid evaluator inputs; no new clamping or input policy is
  invented here.

The first source shape scored 82.755% in the object. Keeping the version-4
sample vectors alive across the point loop restores the retail copy after
the tenth sample and reaches 84.187% in the object / 84.712% linked. GCC
unrolls the ten-sample loop itself. No alignment, inlining, optimization,
or calling-convention attributes were added. Remaining differences include
stack allocation, register selection, and the legacy integration loop;
they do not justify speculative tuning without new evidence.

Verification: target and native builds and all five repository checks pass.
An isolated harness replaces only the evaluator with a deterministic mock
and checks 1,327 complete call/state traces against a retail-offset oracle:
versions -1 through 6, failed opens, nonpositive file counts, zero and full
point arrays, trailing-slot preservation, capacity below file count,
127-byte config paths, pod/non-pod worlds, several integration steps,
and file/math callbacks that change counts and world state. A separate
84-case integration harness uses the real `CalcSplinePoint` on straight
splines and checks lengths, cumulative distances, and untouched bytes.
Both harnesses pass NDK-compiled 32-bit runs and 64-bit ASan/UBSan with
normal global instrumentation. External file services are mocked; this
does not constitute gameplay execution.

## Nearest-obstacle return ABI and arrow rendering

This batch raises linked fuzzy matching from **64.117065% to 64.148240%**:
`GizObstacle_FindNearest` improves **1.98% → 60.98%**, and `RndrArrow`
improves **2.20% → 99.53%**. These are the only changed function scores;
there are no regressions or lost exact matches.

The obstacle query's placeholder incorrectly returned `void`. Retail
returns the selected `GIZOBSTACLE_s *`; its caller in `PushCode` tests the
result and stores it in the game object's obstacle pointer at `0x788`.
The corrected declaration is in `gizobstacles.h`. The existing `-O3` and
symbol name are unchanged.

The query captures the obstacle-array base, but reloads the unsigned
16-bit count after callbacks. It filters by the full integer mode (`-1`
is the wildcard), both enabled/visible progress bits, and the destroyed
runtime bit. A non-null object parameter selects animated average positions
when a set exists; the object itself is not dereferenced. Other candidates
use their stored position. Selection is strictly below `1.0e9f`, so ties
retain the first candidate and NaN distances never win. A null system
returns null without writing the optional distance; an empty/nonmatching
system writes the initial bound. As in retail, animated sets must produce
an average position; the helper's return value is not a fallback selector.
The current partial `PushCode` still lacks this retail caller path; this
batch does not claim to reconstruct the surrounding push state machine.

The first obstacle source form measured 60.778% in the object / 60.984%
linked. A bounded raw-mask versus bitfield experiment generated the same
995-byte function and score. GCC folds the two progress checks into one
mask, unlike retail's two tests; remaining differences also involve loop
unswitching and registers. No compiler or attribute workaround is retained.

`RndrArrow` restores four zero-initialized vectors, the asymmetric arrow
outline, one aspect-ratio snapshot, four in-place rotations, and separate
scale/aspect/translation operations. It submits primitive type 1, format 5,
no material, four colored vertices at z=0, and a final end call. Per-vertex
resolution, stream pointer, and overbrightening state are read afresh.
The existing typed color helper preserves alpha while halving RGB when
required, without touching UV storage. The original `-O2` is preserved.

Combined transform expressions initially produced 938 bytes / 73.702%
object matching. Expressing the three transform passes separately, as in
the retail schedule and nearby quad renderer, yields the original 890-byte
size and 98.932% object / 99.534% linked matching. The remaining 19 linked
instruction differences are color-calculation register/operand choices;
they are not grounds for more speculative tuning.

Verification: target/native builds and all five repository checks pass.
The obstacle oracle passes 134,173 cases on NDK 32-bit and sanitized 64-bit
builds: every progress/runtime byte combination, signed/full-width modes,
strict/tied/NaN/infinite distances, count/base/flag/position mutations,
optional output, and all 65,535 array entries. Its host harness disables
ASan global instrumentation because otherwise unused callback tables keep
the entire obstacle update subsystem linked; the obstacle arrays are heap
allocated and heap/stack ASan plus UBSan remain enabled. The arrow oracle
passes 262,584 cases on NDK 32-bit and full-global ASan/UBSan 64-bit builds:
color-channel and overbrightening bytes, signed angles, floating boundaries,
exact initialization/order, and callback mutations of resolution, aspect,
rotated vectors, and stream cursors. Math/render services are mocked;
no visual or gameplay execution is claimed.

## AI action ownership and gizmo dispatch

This batch raises linked fuzzy matching from **64.148240% to 64.189720%**:

| Function | Before | After |
| --- | ---: | ---: |
| `Action_MoveForward` | 2.04% | 98.82% |
| `Action_PullLever` | 2.23% | 75.99% |
| `Action_UseTechno` | 2.86% | 81.82% |

`Action_MoveForward` had both a global placeholder and a private working
implementation already installed in the AI registry. Promote the working
definition to ordinary external linkage and remove the placeholder. The
registry still points to this implementation; no calling-convention or
optimization attributes are introduced. Its corrected guard uses global
`player`, not `Player[0]`, and checks packet/owner/object in retail order.
It reloads the parameter pointer after each callback, preserves the
two-stage degree-to-angle truncation, and searches for `turn` rather than
`turn=`. The random-direction flag is a 32-bit integer. First-entry yaw,
random consumption, path gating, and captured-object movement are retained.
A valid processor remains a retail precondition after the early guards.
The result is the original 851-byte size, with remaining differences in
stack initialization and register selection.

The two gizmo actions also incorrectly returned `void`, and their script
table entries were null. Both now return `i32` and are bound in
`lego_aiactiondefs`. Retail and linked table contents verify `PullLever`
at index 144 and `UseTechno` at index 146.

`Action_PullLever` parses `lever=` and `instant` before validating the
packet. It preserves an existing target when lookup fails, skips floor
projection for a lever already being pulled, and supports instant setting
of the pull flag, animation frame `0x8000`, and progress 1 without a packet.
Normal operation requests movement, tests the object's lever capability,
uses a strict squared-radius comparison, and presses the current special
button mask. Incapable Free Play characters request a toggle when their
timer goes below zero. Completion requires context `0x4a` and the captured
lever pointer. Sharing the post-parse lever lookup/guards, rather than
duplicating them inside the first-entry block, raises the initial 0%
object result to 75.644%. Remaining block scheduling and register choices
do not justify compiler workarounds.

`Action_UseTechno` captures the object before parsing `name=`, projects the
techno ground position, then requires active/visible/not-complete flags.
It requests movement and, within the strict radius, sets the look target
and calls `GameObjectSetCanUse` with action 2, mode 1, and parameter 0.
Its capability-failure branch preserves Free Play timer/toggle behavior.
`GizTechno_CanUseTechno` really returns 1 in retail; its short existing
body is not a missing reconstruction. The first source form reaches
81.345% in the object. GCC combines the three flag tests; this and block
placement account for much of the remaining difference.

Four function scores improve, including a small register-only change in
the unchanged `Action_FollowCharacter`. Ten unchanged functions lose less
than 0.02 percentage points each from data/GOT operand changes; their
instruction structure is unchanged. No exact matches are lost. The
temporary `Action_FollowPlayer` register regression from the lever-only
build disappears in the complete batch. Source optimization stays `-O3`.

Verification: target/native builds and all five repository checks pass.
NDK 32-bit and full-global ASan/UBSan 64-bit harnesses pass 67,363 forward,
199,508 lever, and 106,419 techno cases. Coverage includes all yaw values,
path bytes, lever flag words, ability/context bytes, techno flag bytes,
signed counts/first-entry values, fractional turn conversion, random
consumption, strict/NaN/infinite distances, timer boundaries, lookup
failures, instant operation without a packet, and callback-driven changes
to parameters, targets, world, radius, object, gamepad, and button masks.
The harness source copies omit only unrelated registry instances so their
unused callback graphs can be discarded; production registry binding is
checked separately. External services are mocked, not gameplay-executed.

## Timing-bar labels and shiny-metal hint traversal

This batch raises linked fuzzy matching from **64.189720% to 64.217150%**:

| Function | Before | After |
| --- | ---: | ---: |
| `TBOPENFN` | 4.36% | 62.78% |
| `TBCLOSEFN` | 3.36% | 91.93% |
| `ShinyMetal_UpdateHint` | 2.04% | 73.88% |

These are the only changed function scores. There are no regressions or
lost exact matches; both source files retain their existing `-O2` setting.

The timing-bar stubs lacked four private twelve-entry label tables. Each
entry contains eleven name bytes and a one-byte type, with a checked
twelve-byte stride. Types 2/3/4/5 select the game/player/AI/draw set. Use
the existing canonical time-bar services and externally owned set IDs;
do not replace them with unused same-named namespace-local HUD stubs.

Opening searches the original count for the first case-sensitive match.
On a miss it reloads the count, checks the twelve-slot limit, copies into
the retail 256-byte local buffer, and ignores empty names. New names are
truncated to ten characters. The count is reloaded after string callbacks
and incremented after the begin callback. Full untruncated names are used
for lookup, so reopening a long name can allocate duplicate truncated
labels; closing the full name will not find that truncated label. Closing
uses a pointer traversal with a snapshotted count and ends only the first
match. Reset changes only counts, not stored labels. Counts in 0..12 and
input names fitting the local buffer remain trusted retail preconditions.

Ordinary `inline` start/end helpers improve matching without forced-inline
attributes. Replacing the close function's index-based traversal with the
retail twelve-byte pointer loop raises object matching from 71.344% to
91.248%. The remaining close gap includes GCC's missing private game-end
clone; do not hand-author a register-ABI clone. Opening remains larger
than retail because of different helper inlining and block generation.

`ShinyMetal_UpdateHint` already had its behavior reconstructed. Its loop
unnecessarily kept the world pointer live across the distance callback.
An explicit count local, refreshed after an unsuccessful distance test,
reproduces retail's register-resident bound between callbacks. It still
reloads both world and count after that call, while preserving the captured
blowup-array base. Object matching improves to 73.332%, with 738 bytes
versus retail's 736. A nested-filter-only experiment produced 0% and was
not retained. Remaining differences are predominantly block placement
and register choices, not missing hint logic.

Verification: target/native builds and all five repository checks pass.
The timing-bar harness passes 15,554 cases on NDK 32-bit and normal-global
64-bit ASan/UBSan builds: all four types, all slot counts, name lengths
0..255, first-match/truncation behavior, invalid types, reset preservation,
callback count/set mutations, and 10,000 stateful oracle operations.
The hint harness passes 23,225 cases on both architectures, including
signed counts, packed filter bits, hint/character/Free Play combinations,
strict/NaN/infinite distances, first selection, palace proximity, and
callback changes to count, world, array, player, hint, and availability.
Only the unrelated `Hints_LSW` dispatch table is excluded from host ASan
registration so its unused callback graph can be discarded; tested arrays,
objects, globals, stack, and heap retain normal instrumentation. External
services are mocked; no gameplay or visual profiling run is claimed.

Additional low-score triage, deferred rather than repeatedly tuned:

- `eduiGradStageSetHSV` already implements HSV conversion. GCC duplicates
  color packing across switch arms. Reusing the canonical HSV helper still
  produces 660 bytes against retail's 446 and 0% object matching; this
  candidate is not retained.
- `WindShear` has a wrong zero-argument placeholder. Retail accepts output
  and input matrices plus signed amplitude and speed/seed arguments. Its
  source ownership belongs with the rendering path, and the LUT angle
  conversion needs an audited policy for large frame-derived values.
  Reconstruct the caller/signature and ownership before implementing it;
  do not invent floating-point guards or matching-only conversion tricks.
- `ImplodeMakeTree` needs missing private heap/length/code helpers and
  shared Huffman state. Its return ABI is also wrong. This is a grouped
  reconstruction, not a useful isolated stub target.
- `NuGScnReadForMultiRender` needs the private graphics reader's ownership
  and scene clone ABI audited together. Retail copies **0x20c** bytes per
  scene, while the current `NUGSCN` declaration ends at **0x1f8**. Audit the
  missing tail and all allocations before adding clone copies; do not
  copy past the current type or expose an artificial private register ABI.

## Socket scene objects and horizontal timing-bar rendering

This batch raises linked fuzzy matching from **64.217150% to 64.254920%**:

| Function | Before | After |
| --- | ---: | ---: |
| `SockParObj` | 3.88% | 99.93% |
| `SockSysSetObjectVisibility` | 8.89% | 100% |
| `SockSysTrackInSplineInfo` | 4.65% | 100% |
| `NuTimeBarSetRenderHorizontal` | 1.50% | 78.29% |

`SOCK + 0xf8` is a pointer to `nuhspecial_s` handles, followed by a
16-bit count at `0xfc`; both offsets are asserted. The parser aligns its
cursor, resolves successive names, advances the handle count only on a
successful lookup, and advances the buffer by the final count. No matches
leave a null object array. Parser callbacks can change the active socket,
scene, cursor, and count; the source retains retail's reloads. The end
pointer is a presence gate, not a capacity check. Sufficient caller-owned
storage and a valid active socket/parser remain preconditions.

Visibility takes `(SOCKSYS *, i32 index, i32 visible)`, not zero arguments.
It captures the selected socket once, then reloads the object-array pointer
and unsigned count after each service call. The track-in query returns
`i32` and takes a system, socket position, optional vector output, and
optional distance output. It preserves the null/-1/valid/track-in gates,
uses the existing real linear spline evaluator, and supplies a local vector
when only distance is requested. Valid socket/segment indices are trusted.
Both functions now match retail exactly under the unchanged default `-O0`.
The parser has only residual local-data operand differences.

The pointer audit also found fixed Android byte counts in `SockSysInit`.
Use `sizeof(SOCKSYS)`, `64 * sizeof(SOCK)`, and natural alignment so the
64-bit structure is fully allocated and cleared. The Android initializer
is instruction-identical before/after this change; the strict capacity
comparison and cursor alignment on failure are retained.

The horizontal profiler's placeholder also had a wrong zero-argument ABI.
It now accepts a set ID, measures label width, maintains unsigned maximum
and recent timing peaks, emits horizontal rectangles and labels, and
performs slot/peak resets. Engine-set suppression happens before font
calls; other sets are required to exist. Width/height rounding, separate
coordinate arithmetic, font/state reloads, and the reset-all special case
for set -1 / slot 0 follow retail. No begin/end-scene calls are invented.
Labels remain trusted internal format strings, as in the original.

The shared unsigned-to-float conversion splits into two sixteen-bit
components. Casting each bounded component to signed `i32` before float
conversion removes GCC's unnecessary unsigned-conversion scaffolding.
This is safe over the complete `u32` domain and does not change the
conversion result. The horizontal function reaches retail's 1,349-byte
size, with remaining register/stack scheduling differences. Its source
stays at `-O2`. The existing vertical renderer changes 5.92% → 5.23% from
conversion/register scheduling, and destruction changes 99.32% → 99.15%
from a register choice. Both diffs were reviewed; no logic changes or exact
matches are lost. Overall, four scores improve and two regress, with two
new exact matches. Do not tune attributes or compiler flags for the gaps.

Verification: target/native builds and all five repository checks pass.
NDK 32-bit and 64-bit ASan/UBSan socket harnesses pass 630,791 cases:
all name-success masks, alignment offsets, empty/65,535-entry/wrapping
counts, guard preservation, callback state changes, all 64 socket slots,
full validity bytes, optional/aliased outputs, real spline interpolation,
floating boundaries, and exact/insufficient allocator capacity. Only the
unrelated parser dispatch table is excluded from ASan registration; all
tested state remains instrumented. The horizontal-renderer oracle passes
334,600 cases on both architectures with normal global instrumentation:
signed counts, engine/initialization/reset combinations, full-width unsigned
timings, exact render/font event traces, callback mutations, and 327,680
unsigned-conversion samples. Services are mocked; no visual/gameplay run
is claimed.

Further read-only triage:

- `Action_BoulderSection` passes the packet, not the script processor, to
  `AIParamToFloat` for `boulder_range=` and `attack_time=`. The callee reads
  processor fields at `+4` and `+0x14`; the bare parameter path passes the
  real processor. Audit this retail ABI/type-punning behavior before
  binding the action. No speculative cast or implementation was retained.
- `BoxTreeRndrRec` needs the owning visibility-tree types and caller to
  reproduce its private EAX/EDX/XMM calling convention naturally. It has
  no reconstructed caller or complete types in its current owner; do not
  implement it as an isolated forced-register-ABI helper.

## Batch 24: texture grid, HTML line graph, and weapon flag audit

Linked fuzzy matching improves from **64.254920% to 64.280630%**:
`MapToGrid` rises from 5.060% to **91.470%**, and `NuHtmlHLineGraph`
from 2.093% to **84.159%**. Two scores improve, none regress, and no
exact matches are lost. Optimization settings and the denominator stay
unchanged.

`MapToGrid` belongs with the texture manager: retail places it between
`NuTexManagerInit` and `NuTexManagerStream`. Remove the character-file
stub and restore the body in `nutex.cpp`. The shared 0x40-byte manager
now names the grid centre, X/Z extents, and signed column/row counts;
size and field-offset assertions preserve the target layout. The
allocator still only aligns/reserves the manager, without clearing it.

Grid coordinates use `(position - centre + extent * 0.5) * (count / extent)`.
Negative values clamp to zero, but the upper test is strictly `>`:
an exact edge remains `count`, while values beyond it become
`count - 0.001`. The two `NuFloor` calls, integer outputs, and fractional
remainders retain retail ordering and alias behavior. Despite its name,
this build's `NuFloor` truncates toward zero. A store-order experiment
did not change code generation; remaining differences are arithmetic
scheduling/register allocation, not a reason for optimization overrides.

The line graph's missing ABI is `(title, width, height, values, count,
maximum, labels)`, not a zero-argument function. Restore its exact
two-stage HTML formatting, integer tick rounding, 16 scan lines per
row, initial eight-step segment, subsequent 17-step segments, zero-slope
one-pixel lines, negative-delta reversal, and final-row slope reset.
Use the real `setpoint`, `setnextpoint`, and `getnextdatapoint` helpers.
The data contract includes `values[count]` as a look-ahead sample for
positive counts, and still reads `values[0]` for empty/negative counts.
Titles and labels are trusted internal diagnostic strings: the retail
256-byte formatter and subsequent format-string writer are preserved,
not made suitable for untrusted input. `maximum` must be nonzero and
the arithmetic/conversions must remain representable.

The fast-weapon audit found a real error independent of the low scores:
`FastWeaponIn` suppresses audio for `gun_on` (0x800), while
`FastWeaponOut` tests `gun_off` (0x400). Replace the misleading shared
constant with distinct names, also used by the existing parser and
automatic transitions. The mask fix leaves fuzzy scores unchanged.
A nested-condition experiment did not improve matching and was not
retained; do not repeat branch permutations without new evidence.

Validation passes on the 32-bit NDK toolchain and 64-bit ASan/UBSan,
with normal global instrumentation throughout:

- **148,226 grid cases** cover signed dimensions/counts, exact and
  near edges, valid infinity/clamping cases, all vector aliases and
  shared index outputs, callback mutations, a 131,073-position sweep,
  and all 16 manager-allocation alignments. NaN/out-of-range
  float-to-integer inputs are outside the retail-defined contract;
  they are not claimed as supported.
- **5,280 HTML graph cases** compare every emitted string and final
  interpolation state, including negative/empty counts, null label
  arrays/entries, rising/falling/constant series, signed widths and
  maxima, rounding, and sample/label changes during writes. Heap sample
  arrays have exactly the required look-ahead capacity.
- **144,480 fast-weapon cases** cover every low 16-bit animation flag
  pattern, signed contexts, all model-mask and weapon-state bytes,
  strict/NaN scale gates, nullable animation entries, Jedi/alternate
  audio precedence, and callback mutations. Sound services are mocked.

Target/native builds and all five repository tests pass. These are
focused output/behavior tests, not a gameplay or visual run.

Further read-only triage:

- `StarFighterAlign` needs the actual `starfighter_s` layout and its
  owning `ProcessStarFighter` call path. The current type is empty;
  retail uses matrix/state fields and a private EAX/EDX/XMM0 convention.
  The partial space-level reset records are not interchangeable with
  that type. Reconstruct the group rather than adding forced ABI casts.
- `CC_sfx_misc` already has the relevant behavior. Retail unrolls six
  slot comparisons, while the established `-O2` build keeps a loop.
  No manual unrolling or flag override was retained.
- `MenuDrawEpisodes` calls the private `DrawEpisodesMenu`, currently
  misplaced as a stub in `hud.cpp`. Audit/consolidate that ownership
  before treating its private register convention as a local mismatch.
- `ReleaseUnreferencedPages_OLD` exposes two issues needing a grouped
  pool audit: the existing 0x400-byte free-list placeholder cannot hold
  256 pointers on a 64-bit host, and the retail rejection path appears
  to revisit the same recycled page instead of advancing. The supplied
  release handlers always succeed, but silently rewriting the rejection
  path would not be faithful reconstruction. No allocator changes were
  included in this batch.

## Batch 25: HTML bar graphs and turret damage/collision

Linked fuzzy matching improves from **64.280630% to 64.359140%**:
seven functions improve, none regress against the published baseline,
and `SphereSphereOverlap` gains an exact match. Optimization settings
and the denominator remain unchanged.

Restore the nine-argument contracts and bodies of `NuHtmlVBarGraph`
(2.542% to **88.475%**) and `NuHtmlHBarGraph` (1.040% to **84.745%**).
The vertical graph uses a 50-pixel axis, an 80-percent height scale,
and descending quarter ticks; the horizontal graph uses a 12-percent
label column, an 88-percent width scale, and ascending quarter ticks.
Both clamp only the upper bar dimension, preserve signed tick rounding,
cycle the supplied palette, and snapshot the value/color before output
callbacks while reloading labels afterward. Negative palette counts
reuse the first entry. Keep the retail two-stage HTML formatting and
the same trusted-string/representable-arithmetic contract as the line
graph. Vertical count/maximum must be nonzero; horizontal count must not
be -1 and its maximum must be nonzero. These were the last two stubs in
`nuhtml.cpp`.

`GizTurrets_Hit` returns a full integer indicating whether it accepted
the hit; the old void stub concealed that contract. Restore signed-byte
health tests and wrapping subtraction, forced destruction, survivor
buzz, random camera judder, animation visibility/role traversal, blowup
and audio dispatch, repeatable pickup rewards, and camera hints.
Callback-sensitive fields and linked-list successors are reloaded at
their retail points. Restore `LEGOHINT_SHOOTCAMERAS` as shared state:
retail initializes it to -1 and game configuration assigns 0x266.
The linked hit score rises from 2.270% to **76.951%**; configuration
initialization rises from 67.250% to **67.711%**.

Restore `GizTurrets_BoltHit` candidate filtering, broad-phase bounds,
reverse sample traversal, first-overlap/strict-nearest selection,
damage dispatch, rumble, deflection, and targeting cleanup. Keep the
otherwise unused bolt-type/cheat calls because they are observable.
Bounds reject with strict greater-than comparisons: unordered/NaN
bounds do not themselves reject a candidate. The candidate-array base
is retained while the system count is reloaded after callbacks.
Retail requires a valid bolt when a candidate is hit, and a valid
`BoltSys` for targeting cleanup; no invented null-bolt behavior is added.
Index-based sample traversal avoids forming an out-of-array pointer
for empty samples.

The shared sphere helper was incorrectly declared `bool`; retail clears
EAX and returns an integer, and its callers test the full register.
Correct the definition and canonical header, removing two conflicting
local declarations. `SphereSphereOverlap` improves from 95% to **100%**,
with a small improvement to `GizmoBlowUp_Hit` as well. Add the canonical
deflected-bolt declaration for the restored call path.

Bolt collision improves from 1.856% to **9.038%**, but its remaining
large mismatch is primarily block order and register allocation.
Two bounded nested-loop/return-shape experiments scored worse (6.932%
object-level versus 8.905%) and were not retained. Adding the real
same-unit caller also changes the hit routine's code generation from
the intermediate 82.78% to 76.95%; the instruction review still shows
the recovered behavior. Do not repeat branch permutations or add
matching-only attributes/optimization overrides to chase these scores.

Validation passes on the 32-bit NDK toolchain and 64-bit ASan/UBSan,
with normal global instrumentation:

- **61,056 cases per bar graph** compare every emitted string, input
  mutations, null labels/palettes, signed dimensions/maxima, palette
  wrapping, out-of-range samples, and callback snapshot/reload ordering.
- **267,530 turret-hit cases** cover byte health and flags, full-width
  damage boundaries, random outcomes, player slots, scores/angles,
  animation roles, nullable sets, rewards/hints, and callback mutations.
- **136,091 bolt-collision cases** exercise the actual hit routine,
  all flag/health/damage bytes, nearest ties, reverse sample order,
  integer overlap results beyond 0/1, owner/type dispatch, NaN/infinite
  bounds, deflection, targeting callbacks, and count/base/state changes.
- **58,249 sphere cases** cover touching/separated bounds, signed radii,
  symmetry, aliases, and NaN/infinite coordinates without input writes.
- The **5,280 line-graph cases** are rebuilt and rerun to check the
  neighboring implementation and shared header.

Target/native builds and all five repository tests pass. External
rendering, collision, audio, pickup, and camera services are mocked in
the focused turret/graph tests; no gameplay or visual run is claimed.

## Batch 26: screen clearing, fade loops, and renderer-owned masking

Linked fuzzy matching improves from **64.359140% to 64.403740%**.
Six functions improve, none regress, and two become exact matches.
The optimization map and matching denominator are unchanged.

- `ClearScreen`: 4.475% to **93.128%**.
- `FadeLoop`: 3.750% to **95.984%**.
- `FadeLoop_SetObj`: 17.500% to **100%**.
- `FadeLoop_UsingObj`: 35.000% to **100%**.
- `FadeLoop_DrawObj`: 10.769% to **77.000%**.
- `RndrMaskScreen`: 1.921% to **93.094%**.

`ClearScreen` draws a normalized four-vertex black quad with 0x80 alpha,
zero depth, and full-range UVs, then restores the coordinate stack.
Use the existing shared primitive helpers, including their float/half
UV representation and callback-sensitive stream cursor. Do not replace
this draw with a render-target clear; they are different operations.

Recover the fade loop's shared scene/special handle and integer
`FadeLoop_UsingObj` return ABI. Object selection stores the scene before
lookup and clears it only on failure; clearing the scene does not erase
the special handle. Object drawing checks existence, applies a 0.125
scale and Z translation of 1, and passes alpha through unchanged.
The retail draw has stack realignment absent from the reconstruction;
no matching-only alignment/calling-convention attribute was added.

The loop fades from 0 to 1 for direction zero and from 1 to 0 otherwise.
Rate is reciprocal duration, or 10 for zero duration. It initializes
`FRAMETIME` from `DEFAULTFRAMETIME`, seeks after frame begin, renders
the optional object and blue/cyan text, runs the optional draw callback,
then ends the scene and enables terrain swapping around frame end.
Store the returned frame time before disabling the swap. Only direction
exactly 1 clears the selected scene and calls `FinishLoop(2)` afterward.
Duration/frame-time inputs must let the seek reach its endpoint;
nonconvergent NaN/negative-time behavior is not silently capped.

`RndrMaskScreen` belongs with the existing `pZClearMaterial` and
`pAlphaMask` statics in `nu3d/nurndr.cpp`, not the gameplay render stub
unit. Remove the misplaced zero-argument stub and recover its contract:
texture ID, clear rectangle, mask rectangle, and coordinate-mode index.
The retail three-entry lookup maps indices 0/1/2 to PS2/normalized/
absolute coordinates. Update the mask texture's low 16 bits, begin a
scene, draw the zero-depth clear rectangle with zero UVs, then draw the
unit-depth textured mask. Each quad separately pushes/restores the
coordinate system; material handles and stream state retain their
retail reload points. Initialization must already have created the
materials, and the mode index must be in range.

Validation passes on the 32-bit NDK toolchain and 64-bit ASan/UBSan,
with normal global instrumentation:

- **23,088 clear-screen cases** verify exact vertex writes, all half-UV
  patterns, untouched storage, cursor changes, and stack restoration.
- **7,974 fade-group cases** verify lookup success/failure and reentrant
  state changes, exact frame/render/callback traces, signed directions,
  zero and positive durations, changing frame times, endpoint colors,
  cleanup, and direct draw alpha including NaN/infinity.
- **69,127 screen-mask cases** cover every low 16-bit texture ID plus
  signed extremes, all coordinate modes, UV patterns, signed/nonfinite
  rectangles, material/cursor callback changes, and stack restoration.

Target/native builds and all five repository tests pass. Rendering and
timing services are mocked; no gameplay or visual run is claimed.

Bounded experiments not retained:

- `GizObstacles_BoltHit` has an audited active-gizmo-array traversal,
  per-sample radius reload, reverse sample selection, and the same
  bolt/cheat dispatch pattern as turrets. A candidate remains 0% at the
  established `-O3`, and an isolated `-O2` diagnostic is also 0%.
  The corresponding turret diagnostic only rises from 8.905% to 10.246%
  object-level. This does not justify an optimization-map change.
  The unvalidated obstacle candidate remains outside the repository.
- Reversing the shared `NuRndrPrimUV` branch order raises ten existing
  callers and lowers two, for +0.0064 aggregate points, but drops
  `NuRndrLine3d` from 21.33% to 3.40% and does not improve either new
  screen function. Restore the original helper instead of retaining
  that cross-caller tradeoff. No exact matches were lost in the trial.

## Batch 27: save-slot rendering and card-warning transitions

Linked fuzzy matching improves from **64.403740% to 64.481260%**.
Six functions improve, none regress, and no exact matches are lost.
The optimization map and matching denominator are unchanged.

- `APIMenuDrawGameState`: 1.308% to **99.268%**.
- `APIMenuDrawMemCardSlots`: 0.844% to **87.167%**.
- `MenuUpdateCardWarning`: 6.885% to **51.033%**.
- The unchanged `MenuDrawSaveConfirm`, `MenuUpdateFormatting`, and
  `DrawMenuEntryEx` also improve slightly. Their before/after diffs only
  change local addresses, jump encoding/alignment, or register choices.

The slot renderer uses the existing four-argument callback ABI: X, Y,
highlight, and slot index. Recover controller/touch colour selection,
unsigned colour-to-float interpolation, signed truncation before byte
narrowing, the 32-byte formatted slot label, and occupied/empty/no-space
messages. Label drawing narrows `MenuA` to a byte, while the following
smart-text call receives its full signed value. Reload scale, alpha,
slot usage, free space, and message colours after the label callback.
The supplied slot must index the six-entry save array, and the formatted
label must fit its retail buffer. No new truncation policy was invented.

The carousel decrements positive left/right slide counters, starts a
ten-frame slide when selection changes, and shows a centred three-slot
window with an extra departing slot while sliding. For at most three
slots it centres the complete list; above six it uses `memcard_slotsused`
as the existing-slot boundary. Preserve the cached window/count and
per-callback reloads of selection, last column, slide state, colours,
scale, alpha, and the slot-info function pointer. The callback is required
when an existing slot is drawn. New-save text uses 0.85 X/Y scale;
navigation arrows use doubled scale and appear only with both slide
counters zero. Arrow colour and scale are captured before the left-arrow
callback, but alpha and the right-arrow availability test are reloaded.
Its second argument is a Y coordinate, not elapsed time.

Card-warning state 0 waits for save/load status exactly 1, then records
the warning/last flow and backs out. State 7 backs out on confirmation
with the select sound; state 3 backs out immediately. Preserve the
previous-state snapshot and callback ordering. The reconstructed
compiler removes the redundant transition-reset tail that retail still
contains, accounting for much of the remaining mismatch. Do not add
volatile state or change optimization settings merely to retain it.

Validation passes with the original 32-bit toolchain and 64-bit
ASan/UBSan:

- **19,424 slot-state cases** cover all colour byte values, selected and
  unselected controller/touch modes, pulse boundaries, signed alpha and
  free-space comparisons, all six slots, geometry, and callback changes.
- **38,536 carousel cases** cover empty/small/extended lists, signed
  slide counters, ten-frame progression, callback replacement, selection
  and count changes, colour interpolation, arrows, and nonfinite geometry.
- **25,088 integration cases** run the actual carousel and slot renderer
  together across all 64 occupancy patterns, controller/touch selection,
  small/extended lists, and available/insufficient storage.
- **249,156 card-warning cases** use the real `BackupMenu`, verify the
  whole menu array, and exercise state/status/input boundaries, four stack
  depths, and state-mutating enter/exit callbacks.

Carousel and warning tests retain normal global instrumentation. Only
the unrelated `GameMenuInfo`/`MenuInfo` callback tables are excluded from
ASan registration in the isolated slot-state and combined-renderer tests;
all tested globals, stack, and heap remain instrumented. Target/native
builds and all five repository tests pass. Rendering is mocked, and no
gameplay or visual run is claimed.

Deferred bounded experiment: `MenuUpdateAutoSaveCancel` has an audited
`MenuASCancelFinished` flag and initially true local `firstTimeIn` byte.
An existing finished flag clears before backing out. On save failure,
first entry disables prompts and sets a five-second delay; positive delay
waits, otherwise prompts are enabled before `NewMenu(1000, -1, -1)`, then
the finished/first-entry flags and delay reset before checking current
confirm/cancel input. NaN delay follows the retry path. Two equivalent
source forms compile to the same poorly aligned 290-byte/0% object body
versus retail's 258 bytes. The unvalidated candidate and its added state
were not retained; revisit with new control-flow/compiler evidence.

## Batch 28: texture-script loading ABI and occlusion statistics

Linked fuzzy matching improves from **64.481260% to 64.508064%**.
Four functions improve, none regress, and one new exact match is gained:

- `NuTexAnimProgReadCFG`: 3.203% to **81.502%**.
- `NuTexAnimProgReadScript`: 87.194% to **100%**.
- `InitTexAnimScripts`: 71.375% to **87.422%**.
- `OcclusionManager::RenderStats`: 4.330% to **99.784%**.

The configuration stub was missing its complete three-argument contract:
path, forward allocation cursor, and scratch-region end. The script
wrapper also incorrectly exposed only two of its four arguments. Recover
the end pointer and integer FPS argument in the shared header and sole
existing caller. `InitTexAnimScripts` passes the current `permbuffer_end`
and truncated `DEFAULTFPS` after path-building callbacks, retains its
four-byte per-script and sixteen-byte final alignment, and leaves a null
input list unaligned. A nonnull empty list still receives final alignment.
The original parser itself ignores its final two arguments; do not invent
FPS processing or bounds checks in `NuTexAnimProgParseFile`.

Recover the initialized `nutexanim_usepakfile = 1` global and the loader's
two paths:

- Archive mode replaces the final extension with `.pak`, narrows file
  size to its low 32 bits, and returns immediately if that size is zero.
  Otherwise load the archive at `(end - size)` rounded down to sixteen
  bytes. Preserve its original start separately from the advancing
  archive-load cursor. Parse the basename `.cfg` item and build a reverse
  list of script basenames immediately below the original archive start.
  Load each script item as a memory file, parse it, then close it. A
  successful archive path does not perform final forward-buffer alignment.
- With archive mode disabled, read the text configuration using full
  script paths and a reverse name list below the supplied scratch end.
  Read scripts in that resulting reverse order and align the forward
  cursor to sixteen bytes. A failed archive load aligns the forward
  cursor before falling through to this text path; a zero archive size
  does not fall back.

Basename extraction tries the last `/` first and only searches for `\\`
if there is no slash. Preserve ignored word-length/item-info returns and
the distinction between parser close and parser destroy. Reusing the
address-escaped archive cursor for list iteration is supported by the
retail stack slot and improves object matching from 79.626% to 80.808%;
the linked result is 81.502%. Remaining differences are stack allocation,
register selection and local control-flow alignment. No optimization
override, volatile state, calling-convention attribute, or helper ABI was
added to chase these differences.

These APIs retain retail's valid-input requirements: generated paths
must fit the 128-byte configuration and 72-byte script buffers (script
names at most 57 bytes with the fixed prefix/suffix), the scratch end
byte is writable, and the arena has room for the archive, name list, and
programs without overlap. Parser/open failures can leave the preexisting
list sentinel untouched; tests supply a valid empty sentinel on those
paths rather than inventing new failure behavior. FPS conversion requires
a representable integer result. Native pointer arithmetic uses full-width
`usize`, while the existing parser retains natural program alignment.

The occlusion statistics renderer uses existing typed state, suppresses
output while uninitialized, disabled, or taking a screen grab, and emits
the original two-pass diagnostic text. It uses PS2 coordinates, 0.7 scale
and point size, position `(112, 112)`, translucent black then cyan, and
calls length scale before height scale for the second-pass offset.
Counters display their signed 32-bit bit patterns; the visible count
subtracts in unsigned arithmetic before conversion. Reload the global
font after each callback, format counters after the first colour call,
and reuse the formatted text after the first print callback. Only the
coordinate and print-mode stacks are restored. The header now declares
the existing screen-grab flag; its ownership and definition are unchanged.

Validation with original-toolchain 32-bit objects and full-global 64-bit
ASan/UBSan passes:

- **16,384 configuration integration cases** exercise archive/text modes,
  failed size/load/open/parser operations, ignored item-info status,
  reverse order, mixed separators, empty words, cursor mutation, and both
  alignments. The harness uses the real script wrapper, parser, command
  table, program initialization, assembler, and linked program list;
  empty and one-instruction programs are checked along with entire arena
  contents and external call traces.
- **22,464 script-caller cases** exercise null/empty/multiple lists, all
  alignment residues, maximum valid path lengths, signed/fractional FPS,
  and callback changes to names, FPS, end pointer, and allocation cursor.
- **65,856 occlusion-rendering cases** exercise all gate combinations,
  signed counter boundaries and wrapping differences, nonfinite scale
  values, and state/font mutations at every external call boundary.

Target/native builds and all five repository tests pass. File/parser
services and rendering are mocked; no asset-loading gameplay or visual
run is claimed. All eleven PR checks were green on the preceding batch.

## Display-list diagnostics and pickup callbacks batch (29)

Baseline: `278d05b4`. Linked fuzzy matching rises from **64.508064% to
64.558060%**. Three functions improve and three regress; no exact matches
are gained or lost:

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `DisplayListPrintItem` | 1.184% | 78.704% |
| `Pup_CollectRedBrick` | 4.719% | 87.843% |
| `Pup_CollectCharKit` | 5.250% | 84.988% |

The display-list diagnostic printer's third argument is a filter count,
not a depth. Recover the two-argument C ABIs of `DisplayListDebugPS` and
`DisplayListPrintItemPS` in the shared header. Retail Android's debug hook
returns zero without touching the supplied buffer; its print hook is
intentionally empty. Both remain exact matches. Do not manufacture an
unknown-type diagnostic when the platform debug hook returns zero: in
that case the existing index text is appended again.

Recover all 28 padded labels from the retail switch table and immediate
stores; each label was checked byte-for-byte. The printer builds its
256-byte text/detail buffers even without an output handle. A zero filter
count selects all types, a negative count selects none, and positive
counts compare the current item type against the array. HTML output
requires selection, a positive debug level, and a nonzero handle. The
platform print hook still runs for every nonzero handle even if filtered
out or debug output is disabled. Preserve the green/blue prefix, low-32-bit
next-pointer text, line break, and conditional black-font reset. Reload
mutable item fields after external calls. The original contains no
console logger call for a zero handle. Separate colour-copy branches and
the boolean filter loop improve the first object candidate from 70.628%
to 78.507%; the linked result is 78.704%. Remaining differences include
stack/register selection and instruction scheduling, not missing labels.

Restore both pickup callbacks in `gizmopickup.cpp`, using the existing
typed world, save, object, and message layouts. Publish the existing
minikit callbacks and panel coordinates through their canonical headers.
Both pickups copy the entire `AddGameMsg_Default` before overwriting their
position/target, scale, flags `0x2112d`, duration, icon, special handle,
tick/end callbacks, and byte `0x4d`. Preserve the stack target vector's
lifetime through the synchronous queue call and native-width pointers.

- Red bricks emit debris `0x62` and the `MK-Pickup` sound before reloading
  `WORLD` and checking the save byte. Area `-1` bypasses the save check.
  Eligible pickups queue icon `0xd2` without a model-active gate, set the
  draw timer even if allocation fails, and reload the world after queuing
  before setting area red-brick state. Pad buzzing is always last. The
  callback does not write the persistent save byte.
- Character kits buzz first, emit debris `0x13`, then gate on the current
  `0xcf` model's active byte. A queued message receives target type six
  only when allocation succeeds. Draw time and the signed count update
  still occur on allocation failure. Counts below ten increment, including
  negative counts; ten and larger remain unchanged. There is no sound call
  in this callback.
- Move the existing static `EndRedBrickMessage` from the unrelated hint
  unit into its pickup owner, in retail definition order. The actual
  callback reference makes its old `__used__` retention attribute
  unnecessary. Its unchanged body remains **100%**: nonzero area state
  becomes two, scale becomes two, then sound, all-player rumble, and
  current-camera judder execute in order.

The two reconstructed pickup bodies have the retail sizes (415 and 408
bytes). Read the complete object diffs; remaining differences are
scheduling and register/stack operands. No optimization-map changes,
inline assembly, calling-convention attributes, or artificial retention
references are used. The aggregate report also records these collateral
changes, with complete before/after diffs reviewed:

- `GameMsg_EndDelay_Game`: 99.212% to 94.773%, register allocation and
  instruction scheduling after removing the misowned static callback.
- `NuDisplayListCaptureSortPriority`: 68.829% to 68.614%, two integer
  register operands after adding the printer body.
- `CollectMinikit`: 68.652% to 68.616%, a literal-address representation.

Retain the correct ownership and larger verified gains rather than adding
matching-only source artifacts to hide these differences.

Original-toolchain 32-bit and full-global 64-bit ASan/UBSan harnesses pass:

- **1,492,992 diagnostic-printer cases** per architecture cover every
  item type, ID classes, signed index/count boundaries, filter hits/misses,
  debug/handle gates, pointer bits, all platform fallback modes, and
  callback changes to type, ID, next pointer, filter, and debug state.
  Exact output and call traces are checked. Platform-supplied text must
  still fit the retail fixed buffers.
- **53,888 pickup cases** per architecture invoke the real pickup-table
  bindings and check the entire copied message, target coordinates, queue
  result writes, and final state. Coverage includes all 72 save slots,
  area `-1`, every save/model byte value, signed counter extremes,
  allocation failures, and callback changes to world, pad, camera,
  coordinates, defaults, and area counters. The relocated completion
  callback is checked separately with signed state boundaries and
  mutations at each external call. Unrelated callbacks retained by ASan's
  real table registration are aborting mocks, not excluded globals.

Target/native builds and all five repository checks pass. External
rendering, input, audio, and queue services are mocked; no gameplay or
visual validation is claimed. All eleven PR checks on the baseline commit
were verified green before publication of this batch.

### Additional low-match triage

`NuTouchInputStick::Render` and the related button renderer require a
shared-helper contract audit before reconstruction. Retail passes a
packed colour converted to float as `RndrUnfilledCircle`'s progress
argument, with integer colour argument 128. The initialized white/grey
values (`0x32ffffff` and `0x32646464`) make the helper's progress-times-360
integer conversion out of range. The current C++ helper therefore has
undefined conversion behavior for these actual caller values, while the
retail x86 instruction yields its integer-indefinite value. Do not silently
swap the arguments to make the image plausible, or restore the callers
without auditing this behavior on target and native platforms. No touch
renderer experiment was retained.

`NuErrorSleep` likewise passes an uninitialized local `va_list` to its
font-print helper in retail. No speculative variadic reconstruction was
made. These findings are recorded to avoid repeated low-yield attempts.

## Customiser texture, selection, and configuration batch (30)

Baseline: `9452302d`. Restore four stubs and correct the shared texture
save/restore signedness contract without changing either owner's `-O2`
configuration. Linked fuzzy matching rises from **64.558060% to
64.571270%**; five functions improve and one regresses. Name initialization
gains an exact match, while the unchanged piece lookup loses its exact
code-generation alignment:

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `Customiser_RestoreModelTextureIDs` | 6.154% | 99.090% |
| `Customiser_Set100PercentPieces` | 12.000% | 78.250% |
| `Customiser_SaveModelTextureIDs` | 63.813% | 78.400% |
| `Customiser_InitNames` | 7.778% | 100% |
| `Customiser_PieceConfig` | 12.000% | 99.971% |
| `Customiser_FindPieceByName` | 100% | 96.594% |

Texture restoration visits both character IDs through `APICharacterLoaded`
and skips missing models. For each of nine categories, a zero saved texture
skips material inspection; a nonzero saved texture is narrowed to its low
16 bits for every matching material, followed by `NuMtlUpdate`. A material's
unsigned tag byte is compared with the category's **signed** tag. Negative
category tags therefore never match an unsigned material tag. Remove the
incorrect unsigned category cast from the existing save function too:
retail explicitly sign-extends the category byte in both functions. Saving
sign-extends each matching texture ID into its 32-bit cache, with the last
matching material winning. Restoration reloads hierarchy, material count,
material pointers, category data, and cached texture values after material
callbacks rather than freezing state across those calls.

Completion selection checks the customiser and save pointers, then scans
each category using a cached signed count and an advancing piece pointer.
The last piece with flag `0x80` selects the primary character, and the last
with `0x100` selects the secondary character. The flags are independent;
both can select the same piece. Empty or negative counts leave the save
untouched, and selected indices retain retail's low-word narrowing. This
does not itself test game completion or unlock pieces. The cached-count
pointer loop is evidenced by retail and improves the first 58.200% object
candidate to 78.250%; remaining differences are register/stack allocation
and addressing, not missing selection behavior.

Name initialization handles two slots in order. Character ID `-1` or
localized-text ID `-1` skips that slot. Otherwise copy the text into the
customiser's 128-byte name buffer and redirect the current `TTab` entry to
that buffer. Keep the text ID captured before the copy, but reload the
global text table afterwards and reload character data for the second
slot. Duplicate character/text IDs consequently allow the second slot to
copy the first buffer and become the final text-table owner. The customiser
must outlive those table references and valid names must fit the buffers.
No extra saved-name/default-name behavior is invented.

Recover the original mutable 84-byte local `Customiser_GameSetting` table
(six records plus sentinel), validating all string pointers and flags
against retail data at `0x621260`:

| Keyword | Model flags | Gameplay flags |
| --- | ---: | ---: |
| `bountyhunter` | `0x01000000` | `0` |
| `jedi` | `0x00000008` | `0` |
| `sith` | `0x0000000c` | `0x00000002` |
| `blaster` | `0x00100080` | `0x40000000` |
| `alreadygothat` | `0` | `0x00000010` |
| `stormtrooperhelmet` | `0` | `0x00040000` |

The configuration callback compares the parser's current word against
each setting case-insensitively, ORs both masks from the first matching
record, and stops. Unknown words leave the piece unchanged. It neither
advances the parser nor clears existing flags. Keep the parser word and
table flags reloadable across comparison callbacks. Names and parser
callbacks are declared through the existing customiser header.

The complete before/after diff for the neighboring
`Customiser_FindPieceByName` has seven changed instructions: two register
pairs and an equivalent conditional/fall-through branch arrangement. Its
body is unchanged and 161 focused lookup cases pass. Retain the coherent
source placement and larger verified improvements rather than introducing
artificial declarations, attributes, or padding to recover that alignment.
The near-exact configuration score differs only in a local table address;
restoration retains two equivalent effective-address expressions and
different local alignment. No layout-only score chase is retained.

Original-toolchain 32-bit and full-global 64-bit ASan/UBSan tests pass on
the actual source, totaling **315,171 cases per architecture**:

- 68,129 restoration cases cover all signed-category/unsigned-material
  byte pairs, both characters, missing models, negative/zero counts,
  all integer texture boundaries including `INT_MIN`, and callback
  mutations to hierarchy, counts, pointers, categories, IDs, and textures.
- 131,096 saving cases cover all tag pairs, all signed 16-bit model IDs,
  missing inputs, zero/negative counts, and signed texture storage.
- 65,567 completion cases cover every availability word, all nine
  categories, independent flags/last-match precedence, null inputs,
  negative counts, and index narrowing through 65,537.
- 161 lookup cases cover all 36 fixture pieces, misses, case folding,
  optional outputs, and a null customiser.
- 48,601 name cases cover sentinel/duplicate IDs, empty through maximum
  127-byte names, both table owners, and callback changes to the global
  tables and second slot.
- 1,617 parser cases cover all retail keywords/masks, case variants,
  unknown/whitespace words, preserved bits, and callback mutation.

Target/native builds and all five repository tests pass. External model
lookup, material updates, and string services are mocked; gameplay and
visual execution are not claimed. All eleven PR checks on the preceding
batch were verified green before publication.

### Deferred availability and file-selector work

`Customiser_PieceAvailable` has a recoverable integer return contract,
but two natural source forms scored 0% and 3.750% in isolated objects
because GCC places the demo branch after the ordinary path instead of
retail's demo-first fall-through. A diagnostic-only `-O3` build of the
second form produced the same score, so this is not evidence for a flag
change. Neither reconstruction nor its provisional header/type additions
is retained. The audited contract for a later attempt is:

- Demo mode returns whether availability bit `0x10` is clear.
- Otherwise flags `0x180` require `Game_100PercentComplete`.
- A non-sentinel signed character ID absent from `InCollectList_Index`
  returns one immediately; a listed but unowned ID returns zero.
- Model flags `0xc` require `Collection_GotAnyOfType(-1, mask)`.
- The signed byte at piece offset `0x11`, unless `-1`, requires
  `Collection_GotAnyOfType(type, 0)`. This byte needs a canonical field
  when the reconstruction is resumed.

`ProcessFileSel3` also has an incorrect void placeholder: retail returns
an integer selection result. Its real file-list helpers and state are in
`core/config/saveload.cpp`, while its stub is in the menu unit; those
owners currently use `-O2` and `-O3` respectively. Reconstruct it with a
coherent file-selector state/header audit, including the missing byte
flags, dimensions, filter strings, and callback, rather than layering new
local declarations into the wrong owner. No file-selector edit is retained.

## File-selector state and language ladder batch (31)

Baseline: `12e6709a`. Restore the file-selector control/state family beside
its real directory, filter, sort, cursor, and pad-repeat helpers in
`core/config/saveload.cpp`. The new shared `fileselect.h` replaces local
incorrect signatures in the menu unit. Live compile commands confirm the
destination uses `-O2 -fomit-frame-pointer` and the former stub owner uses
`-O3`; neither setting is changed. Also replace the device-language
function's hand-written comparison lambda and early returns with the
evidenced cached prefix-selection ladder.

Linked fuzzy matching rises from **64.571270% to 64.628180%**, with six
improvements, two small collateral regressions, **three exact matches
gained and none lost**:

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `ProcessFileSel3(float, nupad_s*)` | 1.603% | 98.374% |
| `StartFileSel` | 7.368% | 100% |
| `FileSelKill` | 26.250% | 100% |
| `ProcessFileSel(float, nupad_s*)` | 23.333% | 100% |
| `RenderFileSel2` | 16.154% | 99.962% |
| `NuIOS_GetDeviceLanguage` | 2.858% | 99.533% |
| `SaveSystemInitialiseEx` | 99.987% | 99.108% |
| `NuQFntCreate` | 56.323% | 56.058% |

`ProcessFileSel2` remains at 100%, now with the correct integer result
forwarded from `ProcessFileSel3`. Return types do not change these symbol
names, so the former void declarations concealed an ABI error.
`StartFileSel` takes five arguments and `RenderFileSel2` takes four, not
the no-argument placeholder signatures.

### File-selector contract

Recover the actual global storage from retail symbol sizes and data:
64-byte title and last-name buffers, 256-byte path/include/exclude buffers,
single-byte active/refresh/volume flags, floating dimensions, and a native
callback pointer. Initialized values are title `Title`, include filter
`*.nup | *.hgp   ` (three trailing spaces), exclude filter
`_PC. | _360. | _PS3.`, X 50, Y 40, and width 248. Existing file-list and
cursor state stays in its original reconstructed owner.

The processor returns zero immediately when inactive, without touching
the pad. Refresh rebuilds the filtered directory, configures the PS2 font
coordinate system, measures text at scale 1/16, clamps the width to a
minimum 240 and adds an eight-unit border. It consumes the repeat helper
once, restores the last-name cursor when nonempty, clears refresh, and
then calls the repeat helper again for the actual input. The double call
is intentional and changes held-input timing.

Movement bits are independent and ordered: down one, up one, down 14,
up 14, down the current file count, and up the current file count. Sort
increments the signed mode with defined 32-bit wrapping, maps exactly
four to zero, saves the selected name, and requests refresh mode two.
The parent button removes one trailing backslash, truncates after the
last remaining backslash, or appends one when none exists; it does not
silently replace a bare path with the root. Parent and confirm can both
execute in the same frame.

Confirm handles volume and directory records separately. A volume clears
the show-volumes byte and replaces the path. A directory named exactly
`..` repeats the parent operation; any other directory appends its name
with the evidenced backslash rules. These paths request refresh one and
clear the remembered name. Every other record type selects a file:
deactivate, copy its name, reload the callback and current cursor, call
the callback if present, and return one even if the callback reactivates
the selector. Do not cache state across string/font callbacks that retail
reloads.

Startup preserves title/path for null arguments, but null filters clear
the corresponding strings. It then sets active one, stores the callback,
and requests refresh two. Kill copies the selected name before clearing
active, without an active-state gate. The legacy processor wrapper runs
the processor and renderer before checking held pad bit `0x10`, then
kills if set; this is not the pressed-button field. The coordinate wrapper
stores X and half Y, calls `RenderFileSel3(0)`, then writes doubled height
before width, including when the output pointers alias.

The **full `RenderFileSel3` renderer remains a stub**. This batch restores
control/state and wrapper behavior, not the selector's visual UI. Valid
inputs still must fit the original buffers: title/selected names up to
63 characters, paths/filters up to 255, and enough space for appended
directory names and separators. No out-of-range float-to-integer behavior
is claimed for the dimension outputs.

### Device-language structural finding

Retail has 23 ordered, case-sensitive prefix tests, including specific
`en-us`, `fr-ca`, `es-mx`, and `pt-br` cases before their three-character
language prefixes. Validate every literal against original read-only data
at `0x562600` and every result against the full disassembly. Any cached
index other than -1 is returned unchanged. A recognized prefix populates
the cache; unknown strings leave -1 so later calls can retry.

An ordinary `memcmp` trial scored 3.642% in the isolated object, while
merely substituting `strncmp` into the old early-return structure scored
0%. Neither is retained. The natural `if (cache == -1)` / `if` / `else if`
assignment ladder followed by one cached return produces the original
1,197-byte structure: GCC itself expands the first 13 comparisons to
`repe cmpsb` and leaves the last ten as `strncmp` calls. No inline assembly,
special builtins, function attributes, or optimization changes are needed.
The retained object scores 98.801%; its linked score is 99.533%, with only
23 local literal-address differences remaining. This is a structural
source lesson, not a reason to hand-code compiler artifacts.

Complete diffs were reviewed for all retained functions. The main file
processor retains equivalent call-argument scheduling and tail-merging
differences. Both collateral regressions have exactly three changed
instructions relative to the baseline: a shifted local constant, a short
conditional jump becoming near, and removal of an alignment instruction.
Their source bodies are unchanged. Retain the verified improvements
without forcing padding or artificial source layout.

### Validation

Original-toolchain 32-bit and full-global 64-bit ASan/UBSan fixtures pass
on the actual source, **544,207 cases per architecture**:

- 87,458 processor/integration cases use the real directory, filtering,
  sorting, cursor, width, and repeat helpers. Cover all 512 relevant input
  combinations, 20 cursor positions, eight path shapes, every active and
  refresh byte, signed mode boundaries, unknown record types, refresh
  failures, callback state changes, remembered selections, NaN/negative
  elapsed times, and valid maximum-length paths/names.
- 8,192 startup cases cover all optional-argument combinations, lengths
  through 255 with the title limited to its own capacity, and callback
  presence/absence.
- 3,528 coordinate-wrapper cases cover full-width signed input
  coordinates, negative and boundary output dimensions within the valid
  conversion range, store order, and aliased output pointers.
- 445,029 language cases cover every byte substituted at all 64 buffer
  positions for each recognized prefix, truncated and non-terminated
  locale buffers, all first-two-byte combinations, arbitrary cached
  integer values, case sensitivity, unchanged input, cache persistence,
  and retry after an unknown locale.

Target/native builds and all five repository tests pass. External
filesystem, font, string, and render services are mocked in the selector
fixtures; gameplay and full UI execution are not claimed. All eleven PR
checks on the preceding customiser batch were green before publication.

## VAO lookup and options-menu behavior batch (32)

Baseline: `68494b63`. Linked fuzzy matching rises from **64.628180% to
64.650734%**. Two functions improve and two unchanged neighbors have small
register-allocation regressions; exact-match counts are unchanged:

| Function | Linked fuzzy before | Linked fuzzy after |
| --- | ---: | ---: |
| `NuIOS_GetOrCreateVAO` | 3.761% | 99.974% |
| `MenuUpdateOptions` | 0% | 65.126% |
| `MenuUpdateSave` | 70.367% | 69.959% |
| `MenuUpdateHints` | 11.641% | 11.500% |

### Renderer lookup layout

Recover the actual LOCAL `g_vaoRecords` array: retail symbol size `0xa000`
at `0x77eb20`, divided into 2,048 records of 20 bytes. Each record stores
two unsigned keys at offsets 0/4, a native `NuVertexFormatPS*` at 8,
the third unsigned key at 12, and the returned four-byte handle at 16.
Keep the three keys opaque: the retained helper has no direct retail call
site establishing more specific parameter meanings. The native record
may grow for pointer width; only the target layout is asserted.

The original unsigned comparison establishes the counter/index type.
Search in insertion order and return the first record matching all four
keys. On a miss, write the four keys into the next slot, increment the
count, and return that slot's **existing** handle. Despite its name,
the Android function neither generates a GL object nor writes/clears the
handle. Reset only clears the count, preserving all records and handles.
Do not invent GL calls or reset-time cleanup. As in retail, the caller
must keep the count within capacity and may not insert into a full table.

The ordinary `-O0` reconstruction reproduces all 371 bytes of instruction
structure. The linked diff has only three local counter-address changes;
the table references pair by their restored real symbol. No attributes,
flags, assembly, or forced linker retention were added.

### Options menu: incomplete behavior behind a zero score

The old implementation was not merely differently optimized. It omitted
the title-level music override, 1.5-second sound-preview state, and preview
positioning; it also changed callback order, cursor feedback, acceptance
fall-through, and controller handling. Recover both real file-local
variables (`opts_sfx_wait`, initialized to 1.5 at `0x617d30`, and
`opts_sfx_i`, BSS at `0x680e70`) in the existing `-O3` menu owner.

- Apply sound volume first. Outside the titles level, use the ordinary
  music-volume setter. On titles, set the music volume directly to the
  queried options volume when the super-option is enabled, or zero.
- Decrement the preview wait on every update, including cancellation.
  At or below zero, add 1.5 once, increment the index with defined 32-bit
  wrapping, map exactly one to zero, and enable this frame's preview.
  This is not a catch-up loop; unordered/NaN waits do not expire.
- Cancel calls `BackupMenu` before resolving/storing the back sound.
  Confirmation alone does not universally exit or play a selection sound.
- Control mode toggles its byte after the selection-sound callback and
  captures the resulting mode before `MechSystems::Get`. A callback change
  to that byte must not change the captured mode. There is no controller
  connectivity gate in the retail update function.
- Surround confirmation toggles the byte and resolves selection audio.
  Without confirmation, an expired preview and enabled surround setting
  play `PickupCoinB` around the current camera position at the configured
  sound-fade radius. The angle is the low 16 bits of the truncated
  `fmod(GlobalTimer.time_elapsed, 8) * 0.125 * 65536`, using the real
  sine/cosine table. Camera/radius are read after the remainder call.
- The volume row copies the old master volume into the selected column
  **before** changing the byte. Left takes precedence when nonzero;
  otherwise right increments only below ten. The other option rows toggle
  music and, outside demo mode, widescreen.
- Preserve the incrementing row cursor through the conditional ladder.
  After a callback, acceptance compares the reloaded selected row with
  that cursor, not an unconditional fixed 4/5. This matters when callbacks
  change the selection. Acceptance compares all 13 options bytes, resolves
  its sound, copies the then-current options, backs out, checks music, and
  stores the saved sound result last.

The canonical audio header now declares the existing volume query and
direct setter. Existing named option fields replace anonymous aliases in
the restored body; no shared layout changes are made. A first candidate
let C++ evaluate `MechSystems::Get` before reading the control-mode byte;
the full assembly review exposed that difference, and an explicit captured
mode corrected it before validation. Remaining code-generation differences
include ESI/EDI assignment, title-music block placement, integer-angle
normalization, and local alignment. Further speculative permutations were
not retained.

Complete before/after collateral diffs contain six register-only operand
changes in `MenuUpdateSave` and four in `MenuUpdateHints`. Neither body's
source changes in this batch. No exact function is lost.

### Validation and follow-up findings

Original-toolchain 32-bit and full-global 64-bit ASan/UBSan tests pass on
the actual source, **456,532 cases per architecture**:

- 27,516 VAO cases cover every count through 2,048, first/middle/last
  matches, every key independently, duplicate first-match precedence,
  null and full-width format pointers, arbitrary existing handles, reset
  preservation, boundary insertion, and 10,000 randomized operations.
  Out-of-capacity insertions are deliberately excluded.
- 429,016 options cases cover every option byte and input combination,
  all menu rows and signed row extremes, both title/demo states, exact
  callback traces, all 65,536 angle values, timer/NaN/infinity boundaries,
  signed counter wrapping, and nine families of callback mutations.
  The real `BackupMenu` and exit callback run in the fixture. Genuine
  constructed polymorphic Mech fixtures retain vptr sanitization.
  Invalid floating-to-integer preview-angle inputs are not claimed.

Target/native builds and all five repository tests pass. External audio
and Mech services are mocked; no gameplay or audible/visual result is
claimed. All eleven checks on `68494b63` were verified green.

The PhoneOS post/pump stubs need their real queue before safe work on their
bodies. Retail has five semaphores, two counters, 128 records of a 24-byte
message plus four-byte token, and a final token field: the LOCAL queue is
`0xe5c` bytes. Its constructor/destructor and queue synchronization must be
reconstructed together, including platform-sized semaphore storage and
the `0x0fffffff` token sentinel. No fake initialized storage or partial
message-queue implementation is added here.

`MenuUpdateHints` also remains behaviorally incomplete: its 423-byte
retail body synthesizes scrolling input, filters `Hints_LSW` by control
mode and available text, clamps per-mode scroll state, and seeks the
displayed position even after cancellation. Its shared two-element LOCAL
target/current arrays must be audited with `MenuDrawHints`, whose current
body is likewise incomplete, before reconstructing that pair.

## Batch 33: hint-menu state and rendering ownership

The paired audit above confirms a shared-state reconstruction, not two
independent wrappers. The retail update/draw bodies are adjacent at
`0x22ed80` (423 bytes) and `0x22ef30` (1,672 bytes), immediately after the
hint-system helpers. They reference LOCAL `updatehints_target_y` at
`0x6b1e38` and `updatehints_current_y` at `0x6b1e40`, each eight bytes,
and the 55-entry `Hints_LSW` table. Both functions move from the menu
catch-all into `legoapi/menus/core/hint.cpp`, alongside that table and
their recovered two-element integer/float arrays. Existing source options
remain unchanged: the old menu owner is `-O3`, the hint owner is `-O2`.
No optimization override, attribute, or build membership change is added.

| Function | Before | After |
|---|---:|---:|
| `MenuUpdateHints` | 11.500000% | 75.798080% |
| `MenuDrawHints` | 6.046205% | 84.412544% |

Linked fuzzy matching rises **64.650734% → 64.684235%**, with two
improvements, no regressions, and no exact-match transitions.

### Recovered behavior

- Active touch confirmation synthesizes up for selected item zero and
  down for every other item. Cancel invokes `BackupMenu` but still counts,
  clamps and interpolates the hints afterward; it does not resolve audio.
  Up has priority even when the target cannot decrement. Down uses the
  original 32-bit wrapping increment.
- Filtering rejects flags `0x2c`; mode zero additionally rejects `0x10`,
  while the alternate mode rejects a missing second text ID. Only non-null
  translated text counts. A target at or beyond the count becomes
  `count - 1`, including `-1` for an empty list. Existing negative targets
  are not independently clamped. `SeekValF` uses speed five and writes the
  mode captured before the callback, even if that callback changes modes.
- Rendering computes the original colour pulse from `HintRGB[2]`, captures
  text/icon X before the first remainder callback, and applies the menu
  entrance slide only to icon X. The up/down arrow text is followed by its
  six corresponding hit-region writes; the height uses one captured aspect
  ratio. It does not emit the placeholder header/back entry.
- Valid translated rows consume spacing and rotation phase even when above
  the upper clipping boundary. Rejected/null rows do not. Text fades between
  Y = 0.2 and 0.3; scrolling stops once the next Y is at most -1.5. Button
  pulse scale is assigned before expanding the translated text into the
  1,024-byte buffer, then reset after drawing. Translation is reloaded after
  the pulse helper, while the selected text ID stays captured.
- Each rendered icon uses the typed `WORLD->lev_objs[0xd4].special`, not a
  fixed byte offset in the native layout. The special pointer is captured
  before the rotation remainder callback. Later rows reload control mode
  and translation table after callbacks. All original angle narrowing is
  preserved without floating-to-small-integer overflow.

The menu table now consumes declarations from the hint header. Shared
arrow-text, colour-table and `BackupMenu` declarations are canonicalized in
their existing subsystem headers without changing any ABI.

The first update candidate used a common loop with mode checks inside it
and scored 43.144% in isolation. The retained mode-specific filter loops
follow the two distinct retail scan paths and score 75.365% in isolation.
The renderer scores 83.538% in isolation. Complete linked diffs were read:
remaining differences are filter-loop/block ordering, register allocation,
equivalent scroll arithmetic, and local data/constant addresses. There is
no further optimization-flag or instruction-shape search in this batch.

### Validation

The actual source passes **151,746 update cases** and **73,489 draw cases**
per architecture using the NDK x86 compiler and full-global 64-bit
ASan/UBSan. Coverage includes all 256 filter bytes, both valid control
modes, every input combination, signed target extremes/wrapping, empty
through 54-row tables, missing IDs/translations, every 16-bit pulse phase,
scroll clipping/fade boundaries including NaN/infinity, captured aspect
ratio, ten callback-mutation families, exact event/state snapshots, and a
1,023-character expanded string. The real hint table remains instrumented;
unrelated services retained through its callback pointers are mocked and
abort if called. Mode values outside the original two-slot contract and
invalid float-to-integer rotation inputs are not claimed as supported.

Target/native builds and all five repository checks pass. Rendering,
remainder/trig services, interpolation and menu backup are mocked in this
fixture; no gameplay or visual output is claimed. All eleven GitHub checks
on the preceding published commit `a078395c` were verified green.

## Batch 34: PhoneOS message queue and semaphore return ABI

Complete the queue prerequisite recorded in batch 32. The existing
`nuphoneos.cpp` stays at `-O3`, with register/post/pump in their original
relative order and LOCAL callback/queue storage. No build flags change.

| Function | Before | After |
|---|---:|---:|
| `NuPhoneOSMessagePost` | 2.763158% | 33.480263% |
| `NuPhoneOSMessagePump` | 3.652174% | 10.278261% |
| `NuThreadQueue<NuPhoneOSMessage, 128>::~NuThreadQueue` | 0% | 100% |

Linked fuzzy matching improves **64.684235% → 64.693110%**: three
improvements, no regressions, one exact match gained, and none lost.

The recovered queue has five real `NuThreadSemaphore` members, two
32-bit counters, 128 message/token records and a final waiting token.
Target assertions cover its `0xe5c` size and the counters/records/token
at offsets `0x50`, `0x54`, `0x58` and `0xe58`. Native semaphore storage
grows naturally; there is no fixed-size pthread placeholder. Construction
sets semaphore capacities to 128, 128, 1, 1, 1, clears the counters,
sets the token sentinel to `0x0fffffff`, and signals all 128 free slots.
Ordinary C++ member destruction reproduces all five reverse-order
destructor calls exactly. The queue cannot be copied.

Posting acquires a free slot, either with a nonblocking try or a wait,
copies the 24-byte message and sentinel token, updates empty/nonempty
notifications when necessary, advances the wrapping write counter, and
signals the occupied slot. A failed nonblocking acquisition returns
without waiting. The optional final wait observes the queue becoming
empty, **not completion of the last callback**. The header now explains
this distinction without changing the exported three-argument ABI.

Pumping first handles pending pause, resume and become-active flags in
that order. Each installed lifecycle callback gets null data, followed by
clearing its flag; later flags/callbacks are reloaded. It then consumes
available queued records, signals matching non-sentinel tokens, advances
the read counter, updates empty/nonempty notifications, and releases the
slot **before** dispatching the callback with the local payload copy.
Null callbacks consume records normally. The three previously absent
lifecycle flag objects are restored with their original four-byte widths.

The shared `NuThreadSemaphore::TryWait` declaration was also wrong:
every retail caller that uses its result tests AL, not EAX. Its canonical
return and local result are now `bool`. The linked semaphore constructor,
destructor, wait, try-wait and signal all remain exact matches. The queue's
cross-thread counters use relaxed atomic loads/stores to preserve the
retail x86 instructions without introducing C++ data races; the existing
semaphores provide record publication and slot-reuse synchronization.
The recovered contract is one producer and one consumer, not an invented
multi-producer lock-free queue.

Full post/pump assembly and object diffs were inspected. The post candidate
has a 28-byte frame instead of retail's 172-byte frame; retail retains
several intermediate aggregate copies absent from the simple recovered
message type. The pump's remaining mismatch is dominated by loop/return
block placement, with its record-copy and notification sequences present.
Do not add redundant copies, manual unrolling or ABI attributes just to
inflate these scores. The original constructor symbol includes its old
filename and unrelated VuVec constants; no filename alias or artificial
vector initialization is added to chase that compiler-generated artifact.

### Validation

- **54,180 trace/state cases per architecture** pass against the NDK x86
  build and full-global 64-bit ASan/UBSan build. They cover all seven
  event IDs, every occupancy 0 through 128, ring/counter wrapping including
  `UINT_MAX`, blocking/nonblocking and drain waits, lifecycle flag and
  callback combinations, matching/nonmatching/sentinel tokens, callback
  registration changes, callback-posted messages and nested pumping.
  Constructor capacities/free-slot signals and reverse destruction order
  are checked separately. Semaphore scheduling is deterministic in this
  trace fixture; records and callback payloads are compared byte for byte.
- A separate host integration fixture uses the actual pthread semaphore
  implementation. Full-queue dropping, a blocked 129th post, acknowledgement
  before callback return, counter wrapping, and **100,000 concurrent FIFO
  messages** pass under both ASan/UBSan and ThreadSanitizer.
- Target/native builds and all five repository checks pass. No gameplay,
  external platform lifecycle wiring, multiple producers, concurrent
  callback registration, invalid event IDs, or invalid message pointers
  are claimed as tested. Lifecycle flag mutation is tested on the consumer
  thread; the queue stress test does not invent asynchronous flag writers.

All eleven GitHub checks on the preceding commit `8d4809f4` were verified
green before this batch was committed.

## Batch 35: autosave callbacks and hub episode initialization

Linked fuzzy matching improves from **64.693110% to 64.701454%**:

| Function | Before | After |
| --- | ---: | ---: |
| `MenuEnterAutoSaveCancel` | 32.308% | **100%** |
| `MenuDrawAutoSaveCancel` | 18.261% | **100%** |
| `MenuUpdateAutoSaveWarning` | 17.500% | **99.375%** |
| `MenuUpdateDoNotRemoveCard` | 22.105% | **99.947%** |
| `MenuInitEpisodes` | 6.761% | **85.254%** |

### Autosave state and callback order

The original four-byte `MenuASCancelFinished` object was absent. Restore it
with the two callbacks that consume it. Entry clears both autosave flags
and the menu's **last row**, not its selection, only when the finished flag
is zero. Drawing similarly gates on the flag, calls `Draw_AUTOSAVECANCEL`,
then reloads `memcard_savefailed` before deciding whether to call `Draw_OK`.
The text helper is genuinely empty in the reference binary; it remains
empty, and its declaration now belongs to the renderer's shared header.

The autosave-warning updater first backs out on any nonzero card-change
flag. It then reads the **original menu pointer's current confirm input**,
even after exit/enter callbacks, and may set the selection sound and back
out a second time without entering the new parent. A combined `else` or an
early return after the first backup would be wrong. The remove-card updater
backs out only after the strict `menu_time > 2.0f` threshold and status 1;
NaN time does not pass that gate.

All four first candidates have the original byte lengths. The two exact
functions require no source-shape tuning. The warning's residual mismatch
is one load/test register pair; the remove-card residual is a literal
address. The previously documented `MenuUpdateAutoSaveCancel` trial is not
repeated: its body remains a stub, so this is not a claim that the complete
autosave-cancellation workflow works yet.

### Episode initializer ownership and widths

`MenuInitEpisodes` at `0x1b3f30` immediately follows the private
`DrawEpisodesMenu` and precedes episode update/draw and the already
hub-owned select-mode callbacks. Move the initializer out of the generic
menu source into `hub.cpp`, preserving both files' existing `-O3` settings.
Its menu-table caller now uses the canonical header declaration.

Restore the original global objects `lastepisodesmode`, `episodesmode`, and
`i_episodes` as **one-byte** state, plus `episodestime` and
`episodesduration` as floats. Initialization sets last mode to -1, time to
zero, mode to zero and duration to 0.6. If save data exists, the selected
episode's first area's **complete byte at offset 0**, not `area_complete`
at offset 1, determines whether the selection falls back to zero. The
episode index and first area ID are sign-extended from 8 and 16 bits.

A non-sentinel level with a non-sentinel episode selects mode 2 and copies
that episode unless a hub-start door is present. A door independently
selects mode 2 even without such a level. The passed menu is untouched.
Shared assertions verify the 28-byte episode stride, first area at offset
4, and level episode at offset `0xae`. No additional index bounds are
invented; callers must supply valid backing tables for the indices used.

The 242-byte candidate versus retail's 264 bytes retains the full behavior.
GCC caches the level episode byte and merges a return path that retail
keeps separate. No volatile loads, branch hints, ABI attributes, padding,
or compiler options are added to imitate those differences.

### Collateral changes and verification

Six functions improve and three regress, with **two exact matches gained
and none lost**. The unchanged `MenuUpdateSelectControls` improves
58.565% to 59.326%. The unchanged hub neighbors change as follows:

- `MenuInitSelectMode`: 99.828% to 99.655%, one register pair.
- `MenuDrawSelectMode`: 37.162% to 33.518%, register allocation and
  instruction scheduling around existing draw-call argument setup.
- `MenuDrawBonusMode`: 88.422% to 88.348%, one register pair plus relocated
  local data/literals. All six differing literal references across these
  renderers retain identical four-byte values before and after.

Full before/after linked diffs for all three regressions were inspected;
their source bodies are unchanged. They were not behavior-tested by the
new focused fixtures, and their remaining reconstruction work is not
claimed complete.

**30,249 autosave cases per architecture** pass NDK x86 and full-global
64-bit ASan/UBSan builds: signed/nonboolean flags, all four stack depths,
both/null exit and enter callbacks, original-menu input mutations, sound
and cursor changes, second backups, exact/adjacent floating thresholds,
NaN/infinite timers, and draw callbacks mutating save/finished state. The
actual `BackupMenu`, `BackupMenuNoFn`, and `MenuRememberCursor` run against
an independent stack/state oracle; render services are mocked.

**143,360 episode initializer cases per architecture** pass NDK x86 and
full-global 64-bit ASan/UBSan builds: every signed episode byte, every save
completion byte, signed area-ID endpoints, null save state, every level
episode byte, missing/present start doors, and level sentinel/valid slots.
Biased, adequately sized fixture tables make the signed index cases valid
C++ accesses; no invalid-pointer or out-of-bounds contract is implied.
An unrelated arcade table receives inert references in the host fixture
so normal ASan instrumentation remains enabled for all globals.

Target/native builds and all five repository checks pass. There is no
gameplay or visual run, and the other episode-menu stubs remain open.
All eleven PR checks on preceding commit `7f44010b` are green.

## Batch 36: stripped renderer diagnostics

Linked fuzzy matching improves from **64.701454% to 64.719740%**.
Five functions improve, one regresses, **four exact matches are gained**,
and none are lost:

| Function | Before | After |
| --- | ---: | ---: |
| `DumpAttributeBindings` | 6.452% | **100%** |
| `MultilineDump` | 12.500% | **100%** |
| `DumpShaderSource` | 4.762% | **96.119%** |
| `DumpProgramSource` | 13.333% | **100%** |
| `DumpShaderAttributes` | 7.692% | **100%** |

These five consecutive functions at `0x293ce9–0x2940a5` belong to the
existing `nurndr_android.c` owner, which remains `-O0`. Although their names
sound like loggers, retail contains **no output calls** here. It retains
the GL queries, allocation, shader lookup, and temporary string processing
after diagnostic logging was stripped. Do not add invented console output
or force the compiler to preserve logging that is absent in the reference.

`DumpAttributeBindings` queries all 16 fixed attribute slots. For enabled
slots it requests buffer binding, size, stride, type, normalization and
pointer in that order. `DumpShaderAttributes` queries the active count,
uses its original static 256-byte `attributeName` buffer, obtains each
location **before** trimming the first `[` suffix, and refreshes the loop
bound after external calls. Its outputs are otherwise unused in retail.

`DumpShaderSource` queries the source length, allocates that exact byte
count from current thread memory with alignment 4, zero-fill flag 1,
empty allocation name and category 0, then fetches the source. It examines
the two signed material shader IDs in order, reloading the current material
between lookups, and stops at the first non-null shader object whose vertex
or fragment handle matches. The recovered key local belonged to the
stripped diagnostic heading. The source then passes through `MultilineDump`
and is freed through a fresh thread-memory lookup.

The material descriptor's adjacent `shader_id` and `shader_variant_id`
now also have a real two-element `shader_ids` array alias. This avoids
out-of-bounds pointer arithmetic from one scalar member to the next while
retaining existing field access and target offsets. Assertions verify the
pair begins at descriptor offset `0x140` and occupies four bytes.

`DumpProgramSource` asks GL for at most two attached shaders and invokes
the actual recovered source routine in order. `MultilineDump` retains the
original const-qualified ABI, but its callers must supply **writable,
nonempty** strings: it searches starting after the first character and
temporarily replaces each later newline with NUL, then restores it. Its
retained line-start local is part of the stripped line-logging code. An
empty one-byte allocation, failed allocation, invalid GL return values,
null current material, or read-only text is not made safe by this batch.
No permissive behavior is invented for those cases.

The first source candidates already recover the original instruction
structure for all four exact functions. `DumpShaderSource` is 305 bytes
versus retail's 309: its key load is direct rather than LEA plus load, and
its loop comparison reads the local directly rather than first loading a
register. No artificial wrapper type, casts, attributes or compiler flags
are introduced to force those two expressions.

### Shared-layout regression and validation

The unchanged `NuMtlRegisterForOverride` drops **97.940% to 93.507%** after
the shader-ID array alias is introduced. Its before/after linked diff is
fully inspected: scratch registers and two independent store positions
change, and GCC reloads the same allocated material pointer around the
shader-ID stores. Its body grows from 241 to 245 bytes. Allocation, lookup,
copy, field values and update calls are unchanged. This function is not
behavior-tested by the new diagnostic fixture; the shared pair itself is.

**192,576 cases per architecture** pass against the actual NDK x86 source
and full-global 64-bit ASan/UBSan build:

- **65,536 binding masks**, checking exact query order and nonboolean
  enabled results for every slot.
- **30,720 attribute cases**, covering lengths 0 through 255, leading,
  trailing, repeated and absent array suffixes, empty/count boundaries,
  callback-mutated counts, full-width program handles, and all 256 bytes
  of persistent name storage after lookup and trimming.
- **24,640 source/program cases**, covering both real routines together,
  zero/one/two attachments, signed shader-ID endpoints, all first/second
  match and null-object combinations, sources through 2,047 characters,
  newline patterns, and callbacks changing material or thread-memory state.
  Exact traces are compared with an independent source-call oracle; GL and
  allocator services are mocked, with exact-sized instrumented allocations.
- **6,144 multiline cases**, verifying complete buffer/canary preservation
  across lengths 1 through 1,024 and byte/newline patterns.
- **65,536 shader-pair cases**, verifying scalar/array alias reads and
  writes for every 16-bit pattern without modifying neighboring fields.

Target/native builds and all five repository checks pass. No live GL
context, logging output, allocation failure, malformed driver output, or
gameplay execution is claimed as tested.
All eleven PR checks on preceding commit `078d3b48` are green.

## Batch 37: movie wrapper and skip callback ABI

Linked fuzzy matching improves from **64.719740% to 64.731090%**, with
three improvements, no regressions and no exact-match transitions:

| Function | Before | After |
| --- | ---: | ---: |
| `Movie_Play` | 4.696% | **99.948%** |
| `Movie_CallBack` | 19.091% | **99.682%** |
| `Movies_ConfigureList` (unchanged body) | 90.759% | **90.823%** |

The movie wrapper's six-argument signature was present but its **integer
return was incorrectly declared void**. The private callback was likewise
a void, forced-emitted stub. Recover both integer returns and the four real
local state objects: `movie_skipped`, `MoviePlayTime`, `MovieFrameTime`, and
`MovieInputFn`. The callback now emits naturally because the wrapper passes
its address to `NuFmvPlayV`; the matching-only `__used__` placeholder is gone.
Their source order follows the callback/configuration/play run in retail,
with the owner's existing `-O3` setting unchanged.

`Movie_Play` clears the skipped flag, constructs regional `movies\\pal\\`
or `movies\\ntsc\\` paths in two 256-byte arrays, appends the name, and
adds `.sub` and `.pss`. It kills audio only when `NOSOUND` is zero, then
sets the frame interval, clears playback time and installs the supplied
input callback or `GamePads_SkipMovie`. These actions still happen when
either allocation-cursor pointer is null; that later gate returns zero.

The successful path forwards the exact recovered tagged option sequence
to `NuFmvPlayV`, including the local float volume pointer, real callback
pointer, start-cursor pointer and dereferenced end cursor. Known pointer
arguments retain native width. No additional meaning is invented for the
remaining numeric tags. The default-input and kill-audio declarations now
come from their canonical module headers.

The return convention is **0 for a failed call or absent cursor pointer,
1 for a nonzero skipped flag, and 2 otherwise**. In particular, the
original CMP/SBB/NOT/ADD sequence returns 2, not 1, when the flag is zero.
Existing trailer callers ignore the return but now see the correct shared
prototype. The callback invokes installed input first, reloads the timer,
accepts nonzero input only at `MoviePlayTime >= 0.2f`, sets the skipped
flag on acceptance, then advances time using the current frame interval.
NaN time does not pass the threshold. Null input still advances time and
returns zero. Previously accepted skip state is not cleared by a later
unaccepted frame.

Two evidence-led source forms were compared. Selecting the region string
inside one call produced CMOV rather than retail's two call sites; the
retained ordinary `if`/`else` restores the original branch. Keeping the
callback's input result and then normalizing it through the threshold
recovers its original shared return path. Final sizes are exactly the
retail **481 bytes** and **96 bytes**. Full linked diffs contain only six
local-state address differences each, plus one literal address in the
callback; do not permute their data to chase those residual fractions.

### Validation and platform limit

**152,113 cases per architecture** pass against NDK x86 and full-global
64-bit ASan/UBSan with a recording playback mock:

- 93,312 wrapper cases cover PAL/audio/input flags, missing and aliased
  cursor pointers, nonboolean backend returns, skip flags, frame runs,
  current-state reloads and callbacks changing input, time, audio or cursor
  state. Reentrant input invokes the real wrapper with absent cursors.
- 481 path cases cover every valid name length through 240 for PAL and 239
  for NTSC, including maximum 255-character resulting paths and nonfinite
  volume values. Oversized names remain outside the recovered contract.
- 58,320 callback cases cover null/custom/default input, signed input
  results, negative and nonfinite time/steps, exact and adjacent 0.2-second
  values, existing skipped flags and callback-mutated/reentrant state.

The same 152,113-case fixture also passes on both architectures with the
**actual `nufmv_android.cpp` backend** in place of the recording mock.
That backend returns 1 immediately and never invokes the callback, just
as the reference Android `NuFmvPlayV` does. Those runs verify wrapper
integration with this platform behavior, **not video playback**. Input,
audio and string services are otherwise mocked; the recording variant
exercises the real movie callback and validates every variadic argument.
Target/native builds and all five repository checks pass. No gameplay or
actual multimedia execution is claimed.

## Batch 38: private command-line parser and bootstrap ownership

`ParseCommandLine()` improves from **5.479% to 99.726%** at its original
291-byte size. Its restored call immediately after `NuAPIInit` also raises
`NuInitHardware` from **19.53% to 19.69%**. The parser, its private
argument cursors, and the real bootstrap caller now share the default-`-O0`
`nu2api/nucore/nuapi.cpp` owner. The misplaced one-stub
`legoapi/core/startup/startup.cpp` is removed; it remains recoverable in Git.
Neither source had an optimization override, and none was added.

Original LOCAL `argc` and `argv` are four-byte BSS objects at `0x74ded4`
and `0x74ded8`. The parser repeatedly checks signed positive `argc` and a
non-null current argument, recognizes `PADRECORD` and `PADPLAY` through
`NuStrICmp`, consumes their following argument into the recording path,
sets the corresponding mode, and then advances the common argument cursor.
The two destination offsets, `nuapi + 0x48` and `nuapi + 0x5c`, now have
target-only layout assertions. Pointers and their arithmetic retain native
width. The final linked diff has only twenty private-state/string address
differences; no instruction-shape work remains in this body.

The reference Android `NuCommandLine` is empty, and no writer to these
private cursors was found in the original text. Consequently the normal
Android path still starts with zero arguments. Do not invent host process
argument plumbing or a platform provider to make the recovered parser run.
The bootstrap call itself is evidenced at `0x2692da`, directly after the
`NuAPIInit` call.

### Compiler-generated score caveat

The linked report changes **64.731090% to 64.729100%**, with two improved
bodies, one regressed ambiguous initializer row, and no exact losses.
Removing the obsolete startup unit removes its incidental header-generated
vector initializer. The report then scores original
`__static_initialization_and_destruction_0` at `0x316eda` as **0% rather
than 99.70%**. This is an ambiguous duplicate-symbol pairing effect, not
removal of scratch initialization: the real `nuscratch_android.c`
initializer remains 373 bytes, with the same six vector constructions and
instruction structure before and after, and its uniquely named
`_GLOBAL__sub_I_nuscratch_android.c` wrapper remains **100%**.

No dummy initializer, include-only translation unit, score filter, or
denominator change is used to hide this artifact. Together with batch 37,
the retained unit raises the published whole score **64.719740% to
64.729100%**: five improved functions, this one artifact regression, and
no exact-match transitions. This is consistent with prioritizing recovered
behavior and coherent source ownership over compiler-artifact layout.

### Validation and bounded scope

**353,280 cases per architecture** pass on NDK x86 and full-global 64-bit
ASan/UBSan: 327,680 argument sequences, 20,480 comparator-mutation traces,
and 5,120 calls through the actual `NuAPIInit`/`NuInitHardware` path.
Tests cover case-insensitive keywords, unknown and prefix tokens, null and
absent following paths, repeated commands, signed count boundaries, and
comparison callbacks changing the cursors or recording state. Complete
state and ordered comparison traces are checked against a separate oracle.
Bootstrap services are mocked and their seventeen-step ordering is checked.

The pre-existing hardware setup dispatcher also routes `NUAPI_SETUP_END`
through its platform fallback and consumes another variadic argument.
The integration fixture supplies that explicit null pointer; this batch
does **not** claim to repair or fully validate the unrelated setup-token
dispatcher. Target/native builds and all five repository checks pass.
No real device bootstrap or pad-recording file I/O was exercised.

## Batch 39: shader-key entry points and mutable digest state

Linked fuzzy matching improves **64.729100% to 64.736800%**, with four
improved functions, **two new exact matches**, and no regressions:

| Function | Before | After |
| --- | ---: | ---: |
| `NuShaderObjectKeyGenerate2` | 16.154% | **100%** |
| `NuShaderObjectKeyGenerate4` | 5.833% | **39.236%** |
| `NuShaderObjectKeySetUberShaderHash` | 7.119% | **100%** |
| `NuShaderObjectKeyGenerate3` | 42.395% | **44.083%** |

The three no-argument placeholders in `pending_stubs.cpp` hid real
parameterized APIs. They now follow `Generate3` in its existing `-O3`
shader-manager owner, matching the consecutive original `0x309620` through
`0x309d09` run. The former catch-all stays `-O2`; no optimization override
is changed. `Generate2` constructs a native-width `ShaderMtlDescFilter`,
forwards descriptor/material pointers and the two scalar arguments into
`internalInit`, then forwards that filter and the pixel-stage argument to
`Generate3`. Its 104-byte linked body matches completely.

The more important state defect was the private **constant** digest in
`Generate3`. Retail instead reads GLOBAL, writable `uberShader2_md5`, a
16-byte object at `0x636e20`, immediately before `uberShader2` at
`0x636e40`. Restore its original bytes and place it alongside the embedded
text in the text's existing `batman.cpp` owner; this does not establish
that provisional asset owner's complete original TU boundary. The shared
shader header declares the object and recovered APIs. Both key builders
now read the same mutable digest, so calling the setter has real effect.

The setter copies sixteen bytes in forward order, accepts exact self-alias,
and uses its original LOCAL, zero-initialized `defaultHash16` for a null
input. **Null resets to zero, not to the initial embedded digest.** Its
ordinary loop naturally emits retail's vectorized non-overlap path and
scalar overlap path. Defining the digest in the manager itself exposed its
alignment to GCC and changed one store instruction; grouping the real
digest with its adjacent shader asset restores the original external-object
access and the complete 186-byte match, without alignment attributes.

`Generate4` restores an output pointer and the two observed 32-bit argument
words, described as flags and selector. It zeros the 104-byte serialized
input, writes the fixed byte at offset 20, copies eight digest bytes at
offset 12, serializes all four flag bytes at offset 4 and the selector's low
nibble at offset 8, then repeats the low three flag bytes at offset 9.
The inverse CRC is stored into the output's high half before the forward
CRC is called and ORed into the low half. Keep that intermediate store and
subsequent output reload. These byte offsets describe a serialized hash
protocol, not native pointer-bearing structure accesses.

The retained `Generate4` is 301 rather than 249 bytes. Full linked review
finds a non-realigned frame, different byte extraction/register scheduling,
and a mask instruction in place of zero extension. No synthetic alignment,
calling-convention attribute, compiler hint or unrolling was added to chase
the remaining score. The existing larger `Generate3` remains incomplete;
only its digest access is repaired in this batch.

### Validation and limits

**307,200 cases per architecture** pass on NDK x86 and full-global 64-bit
ASan/UBSan, both with a recording CRC mock and with the actual `CRC16.cpp`
implementation checked against an independent bitwise CRC oracle:

- 12,288 setter cases cover all byte values, input alignments, exact-sized
  allocations, input preservation, exact self-alias and null resets.
- 262,144 `Generate4` cases verify every serialized byte, signed selector
  boundaries, flag-byte patterns, setter-to-key integration, call ordering,
  digest changes during hashing and CRC callbacks changing the output.
- 32,768 direct/`Generate2` integration cases verify descriptor/material
  pointer forwarding, scalar boundaries, filter lifetime, vertex-versus-
  pixel digest selection and setter resets through the real `Generate3`.
  Filter methods are controlled mocks and other descriptor fields are
  bounded fixtures, **not** a complete material-key reconstruction audit.

Actual CRC runs validate complete key values; callback mutation assertions
belong to the recording variant. Four unused GL table targets are guarded
by aborting mocks so full ASan global instrumentation can remain enabled.
The linked digest's sixteen initial bytes match retail exactly. Target and
native builds and all five repository checks pass. No live shader
compilation, GL context or gameplay execution was tested.
