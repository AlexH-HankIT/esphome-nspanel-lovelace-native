"""Exercise validation with the actual normalized ESPHome task example."""
import copy
from pathlib import Path
import unittest

from esphome import config_validation as cv
from esphome import codegen as cg
from esphome.config import read_config
from esphome.core import CORE

CORE.config_path = Path(__file__).resolve().parents[1] / "ci/tasks.yaml"
config = read_config({}, skip_external_update=True)
assert config is not None
from esphome.components import nspanel_lovelace as component


class StatusIconConfigTest(unittest.TestCase):
    def test_status_constructor_keeps_resolved_icon(self):
        for icon in ("home", "format-list-checks"):
            for setting in (icon, {"value": icon}, {"value": icon, "color": 65535}):
                status = component.SCHEMA_STATUS_ICON({
                    "entity_id": "sensor.nspanel_open_tasks", "icon": setting})
                statement = str(component.get_status_icon_statement(status, component.StatusIconItem))
                resolved = str(component.get_icon(component.get_icon_hex(icon)))
                self.assertIn(", " + resolved, statement)
                if isinstance(setting, dict) and "color" in setting:
                    self.assertIn(", 65535u)", statement)

    def test_configured_icon_and_color_are_explicit_overrides(self):
        # A sensor's empty device_class notification must not replace the
        # configured glyph. The setters mark overrides; constructor defaults don't.
        old_statements = CORE.main_statements
        try:
            CORE.main_statements = []
            component.generate_icon_config({"value": "format-list-checks", "color": 65535},
                                           cg.MockObj("status", "->"))
            statements = "\n".join(str(s) for s in CORE.main_statements)
            self.assertIn("status->set_icon_value(", statements)
            self.assertIn("status->set_icon_color(65535)", statements)
        finally:
            CORE.main_statements = old_statements

    def check_status(self, status, model="eu", side="status_icon_right", error=None):
        candidate = copy.deepcopy(config["nspanel_lovelace"])
        candidate["model"] = model
        candidate["screensaver"].pop("status_icon_right", None)
        candidate["screensaver"][side] = component.SCHEMA_STATUS_ICON(status)
        if error:
            with self.assertRaisesRegex(cv.Invalid, error):
                component.validate_config(candidate)
        else:
            component.validate_config(candidate)

    def test_counts_on_either_side(self):
        for side in ("status_icon_left", "status_icon_right"):
            self.check_status({"entity_id": "sensor.example_count", "show_state": True,
                               "hide_when_zero": True}, side=side)

    def test_existing_icon_only_configurations(self):
        for model in ("eu", "us-l", "us-p"):
            self.check_status({"icon": "home", "alt_font": True}, model=model)
            self.check_status({"entity_id": "binary_sensor.example"}, model=model)
        defaults = component.SCHEMA_STATUS_ICON({"icon": "home"})
        self.assertFalse(defaults["show_state"])
        self.assertFalse(defaults["hide_when_zero"])

    def test_invalid_combinations(self):
        self.check_status({"hide_when_zero": True}, error="requires show_state")
        for entity in (None, "light.example", "binary_sensor.example"):
            status = {"show_state": True}
            if entity:
                status["entity_id"] = entity
            self.check_status(status, error="requires a sensor")
        status = {"entity_id": "sensor.example_count", "show_state": True}
        for model in ("us-l", "us-p"):
            self.check_status(status, model=model, error="EU display only")
        self.check_status(dict(status, alt_font=True), error="combined with alt_font")


if __name__ == "__main__":
    unittest.main()
