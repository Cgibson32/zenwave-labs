# ZenWave Labs — social media autopilot

Goal: brand accounts that post daily with zero time from Corey after setup. No face required. Content comes from the products themselves.

## Accounts
Instagram, TikTok, YouTube (Shorts + long-form), X. Same handle everywhere: `zenwavelabs`.

## Content engine (what gets made automatically)
| Format | Source | Cadence |
| --- | --- | --- |
| 15–30 s "sound reveal" clips: one preset, one knob move, screen capture of the plugin UI + audio | Rendered automatically from the plugin (standalone build + scripted parameter sweeps) | Daily |
| Preset spotlights with before/after audio | Preset list + demo loops | 3× week |
| Release teasers and launch posts | Each product's spec + brand kit | Per release |
| "Sound design tip" text cards and carousels | Written by Claude from the plugin manuals | 2× week |
| Reviewer and user clip reposts | Affiliates and reviewers (with permission) | As available |
| Long-form YouTube: walkthrough per plugin, "making a drop with ZenWave only" | Scripted; screen-recorded; AI or Corey voiceover | 1× release + monthly |

## Pipeline
1. Claude generates a monthly content batch (clips, captions, hashtags, posting times) from the current catalog.
2. Batch lands in a scheduler (Metricool or Buffer) as drafts.
3. Corey's only step: a 15-minute monthly glance at the queue, or none at all (approval can be turned off).
4. Comments and DMs: a Claude agent replies to product questions from the FAQ, flags anything else to email.
5. Weekly report: follower growth, click-throughs to Drift, top clips.

## Rules
- Never mention other artists or brands by name. Describe sounds by style (riddim, deep dub, melodic bass, wonky, halftime).
- No fake engagement, no bought followers, no engagement pods.
- Every post links to the free plugin. The list is the goal, not the follower count.
- Brand voice: bold, direct, technical. One idea per post.

## Setup order
After Drift is built: brand assets → render pipeline for clips → scheduler connected → first 30-day batch queued → launch alongside Drift.
