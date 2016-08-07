#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;
static int16_t secondLastMeasurement = 0;
static int16_t lastMeasurement = 0;
static uint64_t lastImpactTime = 0;
static int16_t impactCounter = 0;
static bool omitData = false;
static char *displayString;
static char *word; // entire sentence
static int32_t letterBits = 0;

static int16_t spaceThresholdCounter = 0;

static int16_t gracePeriodCounter = 0;

static bool fourthValue() {
  if(impactCounter > 0) {
    return --impactCounter == 0;
  } else {
    return false;
  }
}

static void convertToLetter() {
  
  switch(letterBits) {
    case 12: 
      strcat(word, "a");
      break;
    case 2111: 
      strcat(word, "b");
      break;
    case 2121: 
      strcat(word, "c");
      break;    
    case 211: 
      strcat(word, "d");
      break;
    case 1: 
      strcat(word, "e");
      break;
    case 1121: 
      strcat(word, "f");
      break;    
    case 221: 
      strcat(word, "g");
      break;
    case 1111: 
      strcat(word, "h");
      break;
    case 11: 
      strcat(word, "i");
      break;
    case 1222: 
      strcat(word, "j");
      break;
    case 212: 
      strcat(word, "k");
      break;
    case 1211: 
      strcat(word, "l");
      break;
    case 22: 
      strcat(word, "m");
      break;
    case 21: 
      strcat(word, "n");
      break;
    case 222: 
      strcat(word, "o");
      break;
    case 1221: 
      strcat(word, "p");
      break;
    case 2212: 
      strcat(word, "q");
      break;
    case 121: 
      strcat(word, "r");
      break;
    case 111: 
      strcat(word, "s");
      break;
    case 2: 
      strcat(word, "t");
      break;
    case 112: 
      strcat(word, "u");
      break;
    case 1112: 
      strcat(word, "v");
      break;
    case 122: 
      strcat(word, "w");
      break;
    case 2112: 
      strcat(word, "x");
      break;
    case 2122: 
      strcat(word, "y");
      break;
    case 2211: 
      strcat(word, "z");
      break;
    default:
      strcat(word, "?");
      break;
  }
  //displayString = "";
  //sizeof(char something);
  //realloc(displayString, 10);
  //strcpy(displayString, "");
  //APP_LOG(APP_LOG_LEVEL_INFO, "displayString: %s", displayString);
  strcpy(displayString, word);
  APP_LOG(APP_LOG_LEVEL_INFO, "new displayString: %s", displayString);
  letterBits = 0;
}

static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  if(gracePeriodCounter > 0) {
    --gracePeriodCounter;
    return;
  }
  
  // Read sample 0's x, y, and z values
  int16_t x = data[0].x;
  int16_t y = data[0].y;
  int16_t z = data[0].z;

  uint64_t timestamp = data[0].timestamp;
  if (timestamp - lastImpactTime > 1000 && letterBits != 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "lastImpact: %llu, currentTime: %llu", lastImpactTime, timestamp);
    convertToLetter();
    text_layer_set_text(s_text_layer, displayString);
  }
  
  //
  // Detecting dots and dashes
  //
  if (omitData) {
    //do nothing
    omitData = false;
  } else if (fourthValue()) {
    if (z > -700 || lastMeasurement > -700) { // short or long
      APP_LOG(APP_LOG_LEVEL_INFO, "t: %llu, x: %d, y: %d, z: %d :SHORT", timestamp, x, y, z);
      strcat(displayString, ".");
      text_layer_set_text(s_text_layer, displayString);
      letterBits *= 10;
      letterBits += 1;
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "t: %llu, x: %d, y: %d, z: %d :LONG", timestamp, x, y, z);
      strcat(displayString, "-");
      text_layer_set_text(s_text_layer, displayString);
      letterBits *= 10;
      letterBits += 2;
    }
    secondLastMeasurement = lastMeasurement;
    lastMeasurement = z;
    
  } else if (lastMeasurement - z > 1000 || 
             secondLastMeasurement - z > 1000 || 
             (z < -1180 && (lastMeasurement - z > 600))) { // check for impact
    APP_LOG(APP_LOG_LEVEL_INFO, "t: %llu, x: %d, y: %d, z: %d :IMPACT", timestamp, x, y, z);
    impactCounter = 4;
    omitData = true;
    //if (lastImpactTime - timestamp < )
    lastImpactTime = timestamp;
    secondLastMeasurement = z;
    lastMeasurement = z;
  } else {
    //Do not add space after the d, cus it fucks shit up
    APP_LOG(APP_LOG_LEVEL_INFO, "t: %llu, x: %d, y: %d, z: %d", timestamp, x, y, z);
    secondLastMeasurement = z;
    lastMeasurement = z;    
  }
  
  //
  // Detecting space
  //
  if(spaceThresholdCounter <= 0) {
    if(y > 1200) {
      spaceThresholdCounter = 5;
    }
  }
  else {
    if(y < -1100) {
      // Space detected!
      letterBits = 0;
      strcat(word, " ");
      strcpy(displayString, word);      
      text_layer_set_text(s_text_layer, displayString);
      spaceThresholdCounter = 0;
      gracePeriodCounter = 6;
      lastMeasurement = -900;
      secondLastMeasurement = -900;
      impactCounter = 0;
    }
    --spaceThresholdCounter;
  }
  
    //APP_LOG(APP_LOG_LEVEL_INFO, "t: %llu, x: %d, y: %d, z: %d", timestamp, x, y, z);
}
static void init(void) {
	// Create a window and get information about the window
	s_window = window_create();
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
	
  // Create a text layer and set the text
	s_text_layer = text_layer_create(bounds);
	text_layer_set_text(s_text_layer, "Hi, I'm a Pebble!");
  
  // Set the font and text alignment
	text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);

	// Add the text layer to the window
	layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_text_layer));
  
  // Enable text flow and paging on the text layer, with a slight inset of 10, for round screens
  text_layer_enable_screen_text_flow_and_paging(s_text_layer, 10);

	// Push the window, setting the window animation to 'true'
	window_stack_push(s_window, true);
	
	// App Logging!
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Just pushed a window!");
  
  uint32_t num_samples = 1;  // Number of samples per batch/callback

  //setting the sampling rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);
  
  // Subscribe to batched data events
  
  accel_data_service_subscribe(num_samples, accel_data_handler);
  displayString = (char *) malloc(54);
  displayString[0] = '\0';
  word = (char *) malloc(50);
  word[0] = '\0';

}

static void deinit(void) {
	// Destroy the text layer
	text_layer_destroy(s_text_layer);
	
	// Destroy the window
	window_destroy(s_window);
  
  free(displayString);
  free(word);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}



