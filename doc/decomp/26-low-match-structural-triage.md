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
