#include "adc_capture_util.h"

namespace adc_capture_util {

static constexpr uint32_t kUpdateIntervalMillis = 500;

static constexpr uint16_t kCaptureDividerNormal = 5;

static constexpr uint16_t kCaptureDividerAlternative = 25;

struct Vars {
  bool has_data = false;
  bool capture_in_progress = false;
  bool alternative_scale = false;
  // Ignored in has_data is false.
  analyzer::AdcCaptureBuffer capture_buffer;
  bool capture_enabled = true;
  Elapsed elapsed_from_last_update;
};

static Vars vars;

void toggle_scale() {
  vars.alternative_scale = !vars.alternative_scale;
  vars.has_data = false;
  vars.capture_enabled = true;
}

bool alternative_scale() { return vars.alternative_scale; }

const analyzer::AdcCaptureBuffer* capture_buffer() {
  return &vars.capture_buffer;
}

bool capture_enabled() { return vars.capture_enabled; }

void set_capture_enabled(bool val) { vars.capture_enabled = val; }

bool has_data() { return vars.has_data; }

void clear_data() {
  vars.has_data = false;
  vars.capture_enabled = true;
}

bool maybe_update_capture_data() {
  // Is disabled?
  if (!vars.capture_enabled) {
    return false;
  }

  // Do we need to start a new capture?
  if (!vars.capture_in_progress) {
    if (vars.elapsed_from_last_update.elapsed_millis() >=
        kUpdateIntervalMillis) {
      // This will prevent the timer from overflowing, without affecting
      // the logic here.
      vars.elapsed_from_last_update.set(kUpdateIntervalMillis);
      analyzer::start_adc_capture(vars.alternative_scale
                                     ? kCaptureDividerAlternative
                                     : kCaptureDividerNormal);
      vars.capture_in_progress = true;
    }
    return false;
  }

  // Capture in progress but data is not ready yet.
  if (!analyzer::is_adc_capture_ready()) {
    return false;
  }

  // Here where capture is enablled and the last
  // capture request was just completed.
  vars.capture_in_progress = false;

  // Since capture is not in progress now, the content of the
  // asquisition capture buffer should be stable and it's safe to use
  // it.
  const analyzer::AdcCaptureBuffer* acq_capture_buffer =
      analyzer::adc_capture_buffer();

  // Currently we display only captured with a trigger point.
  // This seens to give a better user experience.
  if (!acq_capture_buffer->trigger_found) {
    return false;
  }

  // Here we are comitted to update with the new capture.
  vars.capture_buffer = *acq_capture_buffer;
  vars.has_data = true;
  vars.elapsed_from_last_update.reset();
  return true;
}

void AdcCaptureControls::update_display_from_state() {
  if (vars.capture_enabled) {
   // lv_obj_set_state(run_button.lv_button, LV_STATE_CHECKED);
    status_label.set_text("RUNNING");
    status_label.set_text_color(LV_COLOR_GREEN);
    run_button.label.set_text(ui::kSymbolPause);
    return;
  }

  // Capture is disabled.
  //lv_obj_clear_state(run_button.lv_button, LV_STATE_CHECKED);
  status_label.set_text("STOPPED");
  status_label.set_text_color(LV_COLOR_RED);
  run_button.label.set_text(ui::kSymbolRun);
}

void AdcCaptureControls::sync_button_to_state() {
  if (vars.capture_enabled) {
    lv_obj_set_state(run_button.lv_button, LV_STATE_CHECKED);
    //status_label.set_text("RUNNING");
    //status_label.set_text_color(LV_COLOR_GREEN);
    //run_button.label.set_text(ui::kSymbolPause);
    
  } else {
  lv_obj_clear_state(run_button.lv_button, LV_STATE_CHECKED);
  }
}


bool maybe_update_state_from_controls(const AdcCaptureControls& capture_controls) {
  const bool run_button_checked =
      lv_obj_get_state(capture_controls.run_button.lv_button,
                       LV_BTN_PART_MAIN) &
      LV_STATE_CHECKED;
  if (run_button_checked == vars.capture_enabled) {
    return false;
  }
  vars.capture_enabled = run_button_checked;
  return true;
}

void AdcCaptureControls::setup(ui::Screen& screen) {
  ui::create_label(screen, 100, 130, 0, "", ui::kFontPageTitles,
                   LV_LABEL_ALIGN_CENTER, LV_COLOR_RED, &status_label);
  // Button's symbol is set later by set_displayed_status().
  ui::create_button(screen, 45, 150, ui::kBottomButtonsPosY, "", LV_COLOR_GRAY,
                    ui_events::UI_EVENT_CAPTURE, &run_button);
  lv_btn_set_checkable(run_button.lv_button, true);
  lv_obj_set_state(run_button.lv_button, LV_STATE_CHECKED);
}

}  // namespace adc_capture_util
