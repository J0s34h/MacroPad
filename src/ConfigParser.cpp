#include "../include/yaml/ConfigParser.h"
#include "../include/constants/Config.h"

// Simplified, line-oriented YAML parser. It intentionally supports a
// small subset of YAML sufficient for the config format used by the
// project: top-level `ledState` and `profiles` with nested simple
// mappings and short lists. The goal is readability and reliability.

ConfigParser::ConfigParser() {
  resetConfig();
  valid = false;
  errorMessage[0] = '\0';
}

static void ltrim(char *&s) {
  while (*s == ' ' || *s == '\t')
    s++;
}

static void rtrimInplace(char *s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                     s[len - 1] == '\r' || s[len - 1] == '\n')) {
    s[--len] = '\0';
  }
}

static bool startsWith(const char *s, const char *p) {
  return strncmp(s, p, strlen(p)) == 0;
}

static void copyStringField(const char *src, char *dst, size_t maxLen) {
  if (!src || !dst)
    return;
  // remove surrounding quotes if present
  const char *a = src;
  size_t len = strlen(a);
  if (len >= 2 && a[0] == '"' && a[len - 1] == '"') {
    a++;
    len -= 2;
  }
  if (len >= maxLen)
    len = maxLen - 1;
  strncpy(dst, a, len);
  dst[len] = '\0';
}

void ConfigParser::initLedState(WheelConfig &ledState) {
  ledState.type = WheelSimulation::NONE;
  ledState.hasConfiguration = false;
  ledState.state.solid.brightness = 100;
  ledState.state.solid.color = 0xFFFFFF;
}

void ConfigParser::resetConfig() {
  initLedState(config.globalLedState);
  config.globalLedState.type = WheelSimulation::CLICKY;
  config.globalLedState.hasConfiguration = true;

  config.profileCount = 0;
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
    config.profiles[i].name[0] = '\0';
    config.profiles[i].buttonCount = 0;
    initLedState(config.profiles[i].wheelConfig);
    config.profiles[i].scroll.type = ScrollType::VERTICAL;
    config.profiles[i].scroll.key1 = 0;
    config.profiles[i].scroll.key2 = 0;
    for (uint8_t j = 0; j < MAX_BUTTONS; j++) {
      config.profiles[i].buttons[j].name[0] = '\0';
      config.profiles[i].buttons[j].sequenceLength = 0;
      config.profiles[i].buttons[j].pressType = PressType::SHORT;
      config.profiles[i].buttons[j].delay = 0;
      for (uint8_t k = 0; k < MAX_KEY_SEQUENCE; k++)
        config.profiles[i].buttons[j].sequence[k] = 0;
    }
  }
}

bool ConfigParser::parseFromFile(File &file) {
  resetConfig();
  valid = false;

  if (!file) {
    setError("File is not open");
    return false;
  }

  size_t fileSize = file.size();
  if (fileSize == 0) {
    setError("File is empty");
    return false;
  }

  char *buffer = new char[fileSize + 1];
  if (!buffer) {
    setError("Memory allocation failed");
    return false;
  }

  size_t bytesRead = file.readBytes(buffer, fileSize);
  buffer[bytesRead] = '\0';

  bool ok = parseFromString(buffer);

  delete[] buffer;
  return ok;
}

bool ConfigParser::parseFromString(const char *yamlString) {
  resetConfig();
  valid = false;

  if (!yamlString || strlen(yamlString) == 0) {
    setError("Empty YAML string");
    return false;
  }

  // Make a mutable copy and iterate lines simply
  char *buf = new char[strlen(yamlString) + 1];
  if (!buf) {
    setError("Memory allocation failed");
    return false;
  }
  strcpy(buf, yamlString);

  enum Section {
    NONE,
    GLOBAL_LED,
    PROFILES,
    IN_PROFILE,
    PROFILE_LED,
    PROFILE_SCROLL,
    PROFILE_BUTTONS
  } section = NONE;
  Profile *curProfile = nullptr;
  ButtonConfig *curButton = nullptr;

  int profileIndent = -1;
  int ledIndent = -1;
  int scrollIndent = -1;
  int buttonsIndent = -1;
  int buttonItemIndent = -1;

  char *line = strtok(buf, "\n");
  while (line) {
    char *raw = line;
    // count leading spaces for indentation
    int indent = 0;
    while (*raw == ' ') {
      indent++;
      raw++;
    }
    char *p = raw;
    rtrimInplace(p);
    // strip comments
    char *hash = strchr(p, '#');
    if (hash)
      *hash = '\0';
    rtrimInplace(p);
    if (p[0] == '\0') {
      line = strtok(NULL, "\n");
      continue;
    }

    // Top-level
    if (indent == 0) {
      if (startsWith(p, "ledState:")) {
        section = GLOBAL_LED;
        curProfile = nullptr;
        ledIndent = -1;
        line = strtok(NULL, "\n");
        continue;
      }
      if (startsWith(p, "profiles:")) {
        section = PROFILES;
        curProfile = nullptr;
        profileIndent = -1;
        line = strtok(NULL, "\n");
        continue;
      }
      // unknown top-level key -> ignore
      line = strtok(NULL, "\n");
      continue;
    }

    // GLOBAL LED: expect fields with indent > 0
    if (section == GLOBAL_LED) {
      // parse key: value
      char key[64] = {0};
      char val[128] = {0};
      char *colon = strchr(p, ':');
      if (colon) {
        size_t klen = colon - p;
        if (klen >= sizeof(key))
          klen = sizeof(key) - 1;
        strncpy(key, p, klen);
        key[klen] = '\0';
        char *v = colon + 1;
        while (*v == ' ')
          v++;
        strncpy(val, v, sizeof(val) - 1);
        val[sizeof(val) - 1] = '\0';
        rtrimInplace(val);
      }
      if (strcmp(key, "type") == 0) {
        config.globalLedState.type = parseLedType(val);
        config.globalLedState.hasConfiguration = true;
      } else if (strcmp(key, "brightness") == 0) {
        int v = atoi(val);
        config.globalLedState.state.solid.brightness = constrain(v, 0, 100);
        config.globalLedState.hasConfiguration = true;
      } else if (strcmp(key, "color") == 0) {
        uint32_t c = (uint32_t)strtoul(val, NULL, 0);
        config.globalLedState.state.solid.color = c;
        config.globalLedState.hasConfiguration = true;
      }
      line = strtok(NULL, "\n");
      continue;
    }

    // PROFILES: look for list items
    if (section == PROFILES) {
      if (p[0] == '-') {
        // new profile
        if (config.profileCount < MAX_PROFILES) {
          curProfile = &config.profiles[config.profileCount++];
          memset(curProfile, 0, sizeof(Profile));
          initLedState(curProfile->wheelConfig);
          curProfile->scroll.type = ScrollType::VERTICAL;
          curProfile->buttonCount = 0;
          profileIndent = indent;
          // if inline content after '-'
          char *after = p + 1;
          while (*after == ' ')
            after++;
          if (*after) {
            // treat like key: value on same line
            char *colon = strchr(after, ':');
            if (colon) {
              char key[64] = {0};
              size_t klen = colon - after;
              if (klen >= sizeof(key))
                klen = sizeof(key) - 1;
              strncpy(key, after, klen);
              key[klen] = '\0';
              char val[128] = {0};
              char *v = colon + 1;
              while (*v == ' ')
                v++;
              strncpy(val, v, sizeof(val) - 1);
              rtrimInplace(val);
              if (strcmp(key, "name") == 0)
                copyStringField(val, curProfile->name, MAX_NAME_LENGTH);
            }
          }
          section = IN_PROFILE;
        }
        line = strtok(NULL, "\n");
        continue;
      }
      // ignore unexpected lines
      line = strtok(NULL, "\n");
      continue;
    }

    // IN_PROFILE: fields of the current profile
    if (section == IN_PROFILE && curProfile) {
      // if we've dedented back to or above profileIndent, leave profile
      if (indent <= profileIndent) {
        curProfile = nullptr;
        section = PROFILES;
        continue;
      }
      // parse key: value
      char key[64] = {0};
      char val[128] = {0};
      char *colon = strchr(p, ':');
      if (colon) {
        size_t klen = colon - p;
        if (klen >= sizeof(key))
          klen = sizeof(key) - 1;
        strncpy(key, p, klen);
        key[klen] = '\0';
        char *v = colon + 1;
        while (*v == ' ')
          v++;
        strncpy(val, v, sizeof(val) - 1);
        rtrimInplace(val);
      } else {
        strncpy(key, p, sizeof(key) - 1);
      }

      if (strcmp(key, "name") == 0) {
        copyStringField(val, curProfile->name, MAX_NAME_LENGTH);
      } else if (strcmp(key, "wheel") == 0) {
        section = PROFILE_LED;
        ledIndent = indent;
      } else if (strcmp(key, "scroll") == 0) {
        section = PROFILE_SCROLL;
        scrollIndent = indent;
      } else if (strcmp(key, "buttons") == 0) {
        section = PROFILE_BUTTONS;
        buttonsIndent = indent;
      }
      line = strtok(NULL, "\n");
      continue;
    }

    // PROFILE_LED
    if (section == PROFILE_LED && curProfile) {
      if (indent <= ledIndent) {
        section = IN_PROFILE;
        continue;
      }
      char key[64] = {0};
      char val[128] = {0};
      char *colon = strchr(p, ':');
      if (colon) {
        size_t klen = colon - p;
        if (klen >= sizeof(key))
          klen = sizeof(key) - 1;
        strncpy(key, p, klen);
        key[klen] = '\0';
        char *v = colon + 1;
        while (*v == ' ')
          v++;
        strncpy(val, v, sizeof(val) - 1);
        rtrimInplace(val);
      }
      if (strcmp(key, "type") == 0) {
        curProfile->wheelConfig.type = parseLedType(val);
        curProfile->wheelConfig.hasConfiguration = true;
      } else if (strcmp(key, "maxLeftColor") == 0) {
        curProfile->wheelConfig.state.twist.maxLeftColor =
            (uint32_t)strtoul(val, NULL, 0);
        curProfile->wheelConfig.hasConfiguration = true;
      } else if (strcmp(key, "neutralColor") == 0) {
        curProfile->wheelConfig.state.twist.neutralColor =
            (uint32_t)strtoul(val, NULL, 0);
        curProfile->wheelConfig.hasConfiguration = true;
      } else if (strcmp(key, "maxRightColor") == 0) {
        curProfile->wheelConfig.state.twist.maxRightColor =
            (uint32_t)strtoul(val, NULL, 0);
        curProfile->wheelConfig.hasConfiguration = true;
      }
      line = strtok(NULL, "\n");
      continue;
    }

    // PROFILE_SCROLL
    if (section == PROFILE_SCROLL && curProfile) {
      if (indent <= scrollIndent) {
        section = IN_PROFILE;
        continue;
      }
      char key[64] = {0};
      char val[128] = {0};
      char *colon = strchr(p, ':');
      if (colon) {
        size_t klen = colon - p;
        if (klen >= sizeof(key))
          klen = sizeof(key) - 1;
        strncpy(key, p, klen);
        key[klen] = '\0';
        char *v = colon + 1;
        while (*v == ' ')
          v++;
        strncpy(val, v, sizeof(val) - 1);
        rtrimInplace(val);
      }
      if (strcmp(key, "type") == 0) {
        curProfile->scroll.type = parseScrollType(val);
      } else if (strcmp(key, "key1") == 0) {
        curProfile->scroll.key1 = (uint16_t)strtoul(val, NULL, 0);
      } else if (strcmp(key, "key2") == 0) {
        curProfile->scroll.key2 = (uint16_t)strtoul(val, NULL, 0);
      }
      line = strtok(NULL, "\n");
      continue;
    }

    // PROFILE_BUTTONS
    if (section == PROFILE_BUTTONS && curProfile) {
      // new button item
      if (p[0] == '-') {
        // if dedented, end buttons
        if (buttonsIndent >= 0 && indent <= buttonsIndent) {
          section = IN_PROFILE;
          continue;
        }
        if (curProfile->buttonCount < MAX_BUTTONS) {
          curButton = &curProfile->buttons[curProfile->buttonCount++];
          memset(curButton, 0, sizeof(ButtonConfig));
          curButton->pressType = PressType::SHORT;
          buttonItemIndent = indent;
          // inline fields after '-'
          char *after = p + 1;
          while (*after == ' ')
            after++;
          if (*after) {
            char *colon = strchr(after, ':');
            if (colon) {
              char key[64] = {0};
              size_t klen = colon - after;
              if (klen >= sizeof(key))
                klen = sizeof(key) - 1;
              strncpy(key, after, klen);
              key[klen] = '\0';
              char val[128] = {0};
              char *v = colon + 1;
              while (*v == ' ')
                v++;
              strncpy(val, v, sizeof(val) - 1);
              rtrimInplace(val);
              if (strcmp(key, "name") == 0)
                copyStringField(val, curButton->name, MAX_BUTTON_NAME);
            }
          }
        }
        line = strtok(NULL, "\n");
        continue;
      }
      // within a button block: ensure curButton exists and indent >
      // buttonItemIndent
      if (!curButton || indent <= buttonItemIndent) {
        line = strtok(NULL, "\n");
        continue;
      }
      char key[64] = {0};
      char val[256] = {0};
      char *colon = strchr(p, ':');
      if (colon) {
        size_t klen = colon - p;
        if (klen >= sizeof(key))
          klen = sizeof(key) - 1;
        strncpy(key, p, klen);
        key[klen] = '\0';
        char *v = colon + 1;
        while (*v == ' ')
          v++;
        strncpy(val, v, sizeof(val) - 1);
        rtrimInplace(val);
      }
      if (strcmp(key, "name") == 0) {
        copyStringField(val, curButton->name, MAX_BUTTON_NAME);
      } else if (strcmp(key, "sequence") == 0) {
        // find brackets
        char *s = strchr(p, '[');
        char *e = strchr(p, ']');
        if (s && e && e > s) {
          char tmp[128];
          size_t l = e - s - 1;
          if (l >= sizeof(tmp))
            l = sizeof(tmp) - 1;
          strncpy(tmp, s + 1, l);
          tmp[l] = '\0';
          // parse comma separated
          char *cursor = tmp;
          uint8_t idx = 0;
          while (cursor && *cursor && idx < MAX_KEY_SEQUENCE) {
            // skip leading spaces
            while (*cursor == ' ')
              cursor++;
            char *comma = strchr(cursor, ',');
            size_t toklen = comma ? (size_t)(comma - cursor) : strlen(cursor);
            char tokbuf[32];
            if (toklen >= sizeof(tokbuf))
              toklen = sizeof(tokbuf) - 1;
            strncpy(tokbuf, cursor, toklen);
            tokbuf[toklen] = '\0';
            char *tk = tokbuf;
            ltrim(tk);
            rtrimInplace(tk);
            uint32_t v = (uint32_t)strtoul(tk, NULL, 0);
            curButton->sequence[idx++] = (uint16_t)(v & 0xFFFF);
            if (!comma)
              break;
            cursor = comma + 1;
          }
          curButton->sequenceLength = idx;
        }
      } else if (strcmp(key, "type") == 0) {
        curButton->pressType = parsePressType(val);
      } else if (strcmp(key, "delay") == 0) {
        curButton->delay = (uint16_t)atoi(val);
      }
      line = strtok(NULL, "\n");
      continue;
    }

    // fallback: next line
    line = strtok(NULL, "\n");
  }

  delete[] buf;
  valid = true;
  return true;
}

// We re-use the existing conversion helpers from the header declarations.
WheelSimulation ConfigParser::parseLedType(const char *str) {
  if (!str)
    return WheelSimulation::NONE;
  if (strcasecmp(str, "Clicky") == 0 || strcasecmp(str, "clicky") == 0)
    return WheelSimulation::CLICKY;
  if (strcasecmp(str, "Notchy") == 0 || strcasecmp(str, "notchy") == 0)
    return WheelSimulation::NOTCHY;
  return WheelSimulation::NONE;
}

ScrollType ConfigParser::parseScrollType(const char *str) {
  if (!str)
    return ScrollType::VERTICAL;
  if (strcasecmp(str, "KeyEmulation") == 0 ||
      strcasecmp(str, "keyemulation") == 0)
    return ScrollType::KEY_EMULATION;
  if (strcasecmp(str, "Horizontal") == 0)
    return ScrollType::HORIZONTAL;
  return ScrollType::VERTICAL;
}

PressType ConfigParser::parsePressType(const char *str) {
  if (!str)
    return PressType::SHORT;
  if (strcasecmp(str, "toggle") == 0)
    return PressType::TOGGLE;
  return PressType::SHORT;
}

// Minimal error handling
void ConfigParser::setError(const char *message) {
  strncpy(errorMessage, message, sizeof(errorMessage) - 1);
  errorMessage[sizeof(errorMessage) - 1] = '\0';
  valid = false;
}

// Keep original debug helpers (simple wrappers)
const char *ConfigParser::ledTypeToString(WheelSimulation type) {
  switch (type) {
  case WheelSimulation::CLICKY:
    return "Clicky";
  case WheelSimulation::NOTCHY:
    return "Notchy";
  case WheelSimulation::NONE:
    return "None";
  default:
    return "None";
  }
}
const char *ConfigParser::scrollTypeToString(ScrollType type) {
  switch (type) {
  case ScrollType::VERTICAL:
    return "Vertical";
  case ScrollType::HORIZONTAL:
    return "Horizontal";
  case ScrollType::KEY_EMULATION:
    return "KeyEmulation";
  default:
    return "Unknown";
  }
}
const char *ConfigParser::pressTypeToString(PressType type) {
  switch (type) {
  case PressType::TOGGLE:
    return "Toggle";
  case PressType::SHORT:
    return "Short";
  default:
    return "Unknown";
  }
}