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
