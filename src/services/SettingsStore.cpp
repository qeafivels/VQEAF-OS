#include "SettingsStore.h"

void SettingsStore::begin() {
  prefs.begin("symbian-s3", false);
  // Keep existing NVS namespace and numeric IDs; External must remain 4.
  // v4 makes Midnight the default for new installs and migrates the previous
  // factory Lime default, but retains explicitly selected other/custom themes.
  const uint8_t themeRev=prefs.getUChar("themeRev",0);
  const bool hadTheme=prefs.isKey("theme");
  const uint8_t saved=prefs.getUChar("theme",static_cast<uint8_t>(ThemeId::ModernDark));
  const bool valid=saved<=static_cast<uint8_t>(ThemeId::ModernDark);
  if(!hadTheme||!valid){
    cfg.theme=ThemeId::ModernDark;
  }else if(themeRev<4&&saved==static_cast<uint8_t>(ThemeId::S60Green)){
    // The device's previous shipping default; requested modern replacement.
    // Users may re-select Lime from Themes after the one-time migration.
    cfg.theme=ThemeId::ModernDark;
  }else{
    cfg.theme=static_cast<ThemeId>(saved);
  }
  if(themeRev<4||!hadTheme||!valid){
    prefs.putUChar("theme",static_cast<uint8_t>(cfg.theme));
    prefs.putUChar("themeRev",4);
  }
  cfg.brightness = prefs.getUChar("bright", 90);
  cfg.volume = prefs.getUChar("volume", 80);
  cfg.hour12 = prefs.getBool("hour12", false);
  cfg.wifiAuto = prefs.getBool("wifiAuto", true);
  cfg.lockTimeoutSec = prefs.getUShort("lockSec", 60);
  if (!(cfg.lockTimeoutSec == 0 || cfg.lockTimeoutSec == 30 || cfg.lockTimeoutSec == 60 ||
        cfg.lockTimeoutSec == 120 || cfg.lockTimeoutSec == 300)) {
    cfg.lockTimeoutSec = 60;
  }
  customThemePath = prefs.getString("themeFile", "");
  if (cfg.theme == ThemeId::External && (customThemePath.length() == 0 || customThemePath.length() > 119))
    cfg.theme = ThemeId::ModernDark;
  noteText = prefs.getString("note", "");
}

void SettingsStore::save() {
  prefs.putUChar("theme", static_cast<uint8_t>(cfg.theme));
  prefs.putUChar("bright", cfg.brightness);
  prefs.putUChar("volume", cfg.volume);
  prefs.putBool("hour12", cfg.hour12);
  prefs.putBool("wifiAuto", cfg.wifiAuto);
  prefs.putUShort("lockSec", cfg.lockTimeoutSec);
}

void SettingsStore::selectThemeFile(const String &path) {
  if (path.length() < 1 || path.length() > 119) return;
  customThemePath = path;
  cfg.theme = ThemeId::External;
  prefs.putString("themeFile", customThemePath);
  save();
}

void SettingsStore::selectBuiltInTheme(ThemeId id) {
  if (id == ThemeId::External) return;
  cfg.theme = id;
  customThemePath = "";
  prefs.remove("themeFile");
  save();
}

void SettingsStore::saveNote(const String &text) {
  noteText = text;
  prefs.putString("note", noteText);
}
