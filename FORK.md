# Features added by this fork

This fork builds on [olicooper/esphome-nspanel-lovelace-native](https://github.com/olicooper/esphome-nspanel-lovelace-native). The upstream installation instructions remain in [README.md](README.md).

Use this fork as your external component source to obtain these features:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/AlexH-HankIT/esphome-nspanel-lovelace-native
      ref: dev
    components: [nspanel_lovelace]
```

## Screensaver entities

Exactly four configured entities can replace the four weather forecast slots. The main weather item can still be displayed. Forecast subscription and incoming forecast processing are disabled when these entities are configured. Their state and attribute changes refresh the screensaver.

```yaml
nspanel_lovelace:
  screensaver:
    time_id: homeassistant_time
    entities:
      - entity_id: sensor.room_temperature
        name: Temperature
      - entity_id: switch.desk_lamp
        name: Desk
      - entity_id: binary_sensor.front_door
        name: Door
      - entity_id: sensor.open_tasks
        name: Tasks
```

This is a display feature; completing tasks uses the task card below.

## Timer and entity state fixes

- The timer popup parses ISO8601 finish times with timezone offsets, calculates remaining time against the panel clock, and updates the countdown every second. Configure a screensaver `time_id` to supply that clock. The timer's `remaining` attribute is a fallback when a valid clock/finish time is unavailable.
- Screensaver on/off entities display translated text rather than numeric control values. Binary sensors and alarm entities combine their state-specific icon handling with translation; opening sensors use open/closed text.

## Task cards

`cardTodo` uses the existing `cardEntities` TFT page. It needs the compatible TFT already described in README; it does not require a new TFT image. A hardware walkthrough is still required to verify layout on your panel model. See [tasks-example.yaml](tasks-example.yaml) for a complete ESPHome example.

```yaml
nspanel_lovelace:
  locale:
    language: de
  cards:
    - type: cardTodo
      id: maintenance
      title: Wartung
      sleep_timeout: 30
      todo_lists: [todo.maintenance]
      completion_script: script.nspanel_complete_task
      refresh_script: script.nspanel_sync_tasks
```

`id` is required and must be unique among cards. It is the public identifier used by the Home Assistant bridge; it is independent of the generated internal TFT UUID. `todo_lists` is a nonempty allowlist of list entities. The two script settings default to the names shown above.

Each task occupies two screen rows: its title and completion button, then due status and last-done text. EU and US landscape panels show two tasks per page, US portrait shows three. The heading shows the task page number. Left/right navigation moves through task pages and then through the surrounding configured cards. Completed items are retained in HA and removed from the open-task display after confirmation.

### Home Assistant owns task state

The integration has two directions:

1. HA reads `todo.get_items`, looks up last-completion information, filters tasks for the panel, and pushes a snapshot over the ESPHome native API.
2. The panel calls a HA script with `todo_entity` and `uid`. The script marks that exact occurrence `completed`, updates maintenance bookkeeping, and publishes the new snapshot.

There is no MQTT broker, HA access token stored on the panel, or scheduling logic in firmware. The native ESPHome API connection supplies authentication. In HA's ESPHome integration, enable **Allow the device to perform Home Assistant actions** for the panel so completion and refresh requests can execute. Custom API action support is enabled automatically when a task card is configured.

HA entity state alone contains only the open-item count. The bridge must read individual items; a count cannot provide titles, identities, or completion dates.

### Snapshot action

Task cards register the device action `set_task_data` with two string arguments, `card_id` and `payload`. Find the exact generated action name in HA's action picker; names depend on the ESPHome device name.

```yaml
action: esphome.example_panel_set_task_data
data:
  card_id: maintenance
  payload: >-
    {"version":1,"items":[{"entity_id":"todo.maintenance","uid":"occurrence-uid",
    "summary":"Example task","due_text":"Due today",
    "last_done_text":"Last 11.09.2026","overdue":false}]}
```

All six item fields shown are required. The supplied HA bridge also includes an `ok` boolean at the top level so native HA templating keeps empty/error payloads as JSON strings; additional top-level fields are ignored. `entity_id` must belong to the card's allowlist; `uid` identifies an occurrence within that list. The bridge supplies already formatted display strings, in the HA timezone. An empty `items` array means no open tasks. To report a synchronization failure, send `{"version":1,"error":"reason"}` instead. Invalid versions, duplicate identities, missing/wrong-type fields, unknown lists, more than 32 tasks, and payloads larger than 32 KiB are rejected while retaining the last snapshot internally.

Display text is shortened by Unicode code points and protocol delimiters/control characters are replaced by spaces. This does not alter HA titles or UIDs. German and English task-control labels are supplied; existing/custom translation files without the optional `todo_*` keys fall back to English.

The panel refreshes on task-card entry and after HA API reconnection. Completion leaves the occurrence visible with a pending indicator until a snapshot confirms it has disappeared. Repeated presses are ignored while pending. After 10 seconds without confirmation, the control returns and the heading reports an unconfirmed action. Removed/recreated occurrences receive new touch tokens, so delayed taps cannot complete a different row.

Snapshots become stale after 120 seconds, and controls are disabled when the HA API disconnects, data is stale, or synchronization fails. Loading, empty, stale, and synchronization-error states are distinct. Publish regularly, including when the item count has not changed: titles, due dates, and completion metadata can change independently of that count. The supplied local bridge reconciles every 60 seconds.

### HA configuration and recurring chores

The bridge can be supplied by ordinary HA packages/scripts; a custom HA integration is not required. Installation-specific generated packages live in the ignored `.local/ha-packages/` directory when prepared locally. Their application instructions are in that directory's README; they are not distributed in Git. They must be reviewed and applied by the installation owner.

A shared chore registry connects stable keys to titles, timestamp helpers, optional cycle-counter baselines, and optional room/panel restrictions. New managed items contain a marker such as `[nspanel_task:example.task]` in their descriptions. Legacy title matching lets retained history participate in backfill. Ordinary unmapped tasks remain completable without changing maintenance helpers.

- A checklist check and a panel press both mark an occurrence completed. A completion handler updates the last-done helper from the recorded completion timestamp, rather than the time a later reconciliation happens.
- Cycle-based chores become due when `current counter - baseline >= interval`. Each chore has its own persistent baseline; counters are not reset after cleaning.
- Migration initializes baselines from current valid counters and seeds processed completion IDs. This starts a fresh interval at installation while preserving real completion dates. It deliberately does not infer completion-time counts from creation-time task descriptions.
- Invalid counters defer baseline capture. A decreasing counter establishes a fresh baseline without recording a completion. Manually recorded early maintenance creates and completes an occurrence, so history and scheduling agree.
- Monthly chores add calendar months in HA's timezone, clamping the day to the end of a shorter month. A never-completed chore is initially due immediately. There is at most one open managed occurrence per chore.
- Keep one maintenance list and map chores to optional areas and panel IDs. An empty restriction means any panel; when both restrictions exist, both must match. A profile pairs a panel ID/area with its device action name and card ID.

Completed occurrences are history. Removing them prevents future recovery of their timestamps; persistent timestamp/baseline helpers preserve the latest processed completion even after old items are removed. Keep those helpers in backups. Undoing a checklist completion does not roll maintenance history or counter baselines backwards; record a genuinely new completion to move them forward.

## Screensaver status counts (EU)

Either `status_icon_left` or `status_icon_right` can display an icon alongside a non-negative integer sensor state. The status area is separate from the clock and the four forecast/configurable entity slots. This is generic entity rendering; the firmware does not calculate or filter tasks for the icon.

```yaml
nspanel_lovelace:
  screensaver:
    status_icon_right:
      entity_id: sensor.nspanel_open_tasks
      icon:
        value: format-list-checks
        color: 65535
      show_state: true
      hide_when_zero: true
```

Both options default to `false`, preserving existing icon-only configurations. `show_state` requires a `sensor` entity and the EU display; it cannot be combined with `alt_font: true`. `hide_when_zero` requires `show_state`. Counts above 99 display as `99+`. Units, negative values, decimals and other non-integer states are not displayed. Unknown/unavailable/invalid states, initial loading and HA API disconnection display the icon plus a grey `?`; only a received zero count is hidden. The sensor remains responsible for reporting stale data as unavailable. Existing tap behavior is unchanged, and the compatible TFT needs no replacement.

For a panel-specific task count, have HA derive the sensor from the same filtered open items used for that panel's task snapshot, rather than from the household list's total. Publish the sensor separately from device delivery so an offline panel does not prevent HA counts from updating. The prepared local HA bridge supplies these sensors with a 120-second freshness limit, checked every ten seconds. It publishes unavailable counts on synchronization errors or snapshots exceeding the existing task limits.

## Verification and rollout

Run the host-side identity/confirmation tests:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -Icomponents/nspanel_lovelace \
  tests/task_model_test.cpp -o /tmp/task-model-test
/tmp/task-model-test
```

CI compiles the advanced and task-card examples against stable ESPHome. The task-card compile includes custom API action registration and code generation. Use `esphome compile ci/tasks.yaml` locally after supplying the example secrets.

Validate HA packages in a separate configuration with simulated counters and a test list before application. On a panel, verify: two-row labels and German text; pagination at both boundaries; completion from HA and the panel; synchronization across two panels; unchanged last-done dates on repeat requests; and disabled controls after disconnect. A completion timeout should leave the task open and available for retry, never claim success.

If a card stays loading, check its public ID, the configured device action, and whether the bridge profiles are populated. If completion is unconfirmed, check HA's device-action permission and the completion script trace. An error after publishing often means a list allowlist mismatch or invalid snapshot. A stale card means the bridge has not delivered a fresh snapshot within two minutes.
