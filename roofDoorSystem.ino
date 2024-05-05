typedef enum {OPENED, IS_CLOSING, CLOSED, IS_OPENING, DELAYED, ERROR_STATE} t_doorStates;

#define OUT_DO_RUN    2     /* informacija da li treba pustiti struju do motora */
#define OUT_DO_SWITCH 3     /* po automatizmu, motor "otvara" vrata, ako treba da ih "zatvori" šalje se ova informacija */

#define OUT_DIODE_YELLOW_OPEN   4     /* informacija da je arduino dozvolio otvaranje vrata, struja na diodu */
#define OUT_DIODE_YELLOW_CLOSE  5     /* informacija da je arduino dozvolio zatvaranje vrata, struja na diodu */
#define OUT_DIODE_RED_ERROR     6     /* informacija da je arduino primetio grešku, struja na diodu */

#define IN_TASTER_UP    8        /* neki od tastera za otvaranje pritisnut */
#define IN_TASTER_DOWN  9        /* neki od tastera za zatvaranje pritisnut */
#define IN_SWITCH_UP    10       /* kontrolni prekidac za otvoreno stanje */
#define IN_SWITCH_DOWN  11       /* kontrolni prekic za zatvoreno stanje */

#define ERROR_TIMEOUT 30000       /* TODO proveriti koliko inace vremena treba da se otvori!!! (u miliskenudama) */
#define SIGNAL_DELAY_TIME 300
#define SWITCH_DELAY_TIME 200

t_doorStates doorState = CLOSED;
t_doorStates stateBeforeStopped = IS_CLOSING;

int inTasterUpState = LOW;
int inTasterDownState = LOW;
int inSwitchUpState = LOW;
int inSwitchDownState = LOW;

long time_reset = 0;
long time_check_up = 0;
long time_check_down = 0;

bool errorConditionSatisfied = false; 

void readInputs(void);
void checkErrorCondition(void);
int checkTasterState(int, long*);
bool isTimeoutPassed(long, long);

void handleClosedState(void);
void handleOpenedState(void);
void handleIsClosingState(void);
void handleIsOpeningState(void);
void handleStoppedState(void);
void handleErrorState(void);

void readInputs(void)
{
  int inTasterUpRead = digitalRead(IN_TASTER_UP);
  int inTasterDownRead = digitalRead(IN_TASTER_DOWN);

  inSwitchUpState = digitalRead(IN_SWITCH_UP);
  inSwitchDownState = digitalRead(IN_SWITCH_DOWN);

  inTasterUpState = checkTasterState(inTasterUpRead, &time_check_up);
  inTasterDownState = checkTasterState(inTasterDownRead, &time_check_down);

  return;
}

void checkErrorCondition(void)
{
  errorConditionSatisfied = !(isTimeoutPassed(time_reset, ERROR_TIMEOUT));
  return;
}

int checkTasterState(int tasterRead, long* time_check)
{  
  if (LOW == tasterRead)
  {
    if (0 == *time_check)
    {
      *time_check = millis();
      return HIGH;  
    }
    else
    {
      bool is_pressed_long_enough = isTimeoutPassed(*time_check, SIGNAL_DELAY_TIME);
      if (false == is_pressed_long_enough)
      {
        return HIGH;  
      }
      else
      {
        return LOW;      
      }
    }
  }
  else
  {
    *time_check = 0;
    return HIGH;  
  }
}

bool isTimeoutPassed(long time_since, long timeout)
{
  bool time_is_overflow = false;  
  long time_now = millis();
  long time_margin = time_since + timeout;

  if (time_margin < time_since)
  {
    time_is_overflow = true;
  }

  if (time_margin <= time_now)
  {
    if (false != time_is_overflow && time_margin >= time_now)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return true;
  }
}

void handleClosedState(void)
{
  digitalWrite(OUT_DO_RUN, LOW);
  digitalWrite(OUT_DO_SWITCH, LOW);
  digitalWrite(OUT_DIODE_YELLOW_OPEN, LOW);
  digitalWrite(OUT_DIODE_YELLOW_CLOSE, LOW);
}

void handleOpenedState(void)
{
  digitalWrite(OUT_DO_RUN, LOW);
  digitalWrite(OUT_DO_SWITCH, LOW);
  digitalWrite(OUT_DIODE_YELLOW_OPEN, LOW);
  digitalWrite(OUT_DIODE_YELLOW_CLOSE, LOW);
}

void handleIsClosingState(void)
{
  digitalWrite(OUT_DO_RUN, HIGH);
  digitalWrite(OUT_DO_SWITCH, LOW);
  digitalWrite(OUT_DIODE_YELLOW_OPEN, LOW);
  digitalWrite(OUT_DIODE_YELLOW_CLOSE, HIGH);
}

void handleIsOpeningState(void)
{
  digitalWrite(OUT_DO_RUN, HIGH);
  digitalWrite(OUT_DO_SWITCH, HIGH);
  digitalWrite(OUT_DIODE_YELLOW_OPEN, HIGH);
  digitalWrite(OUT_DIODE_YELLOW_CLOSE, LOW);
}

void handleStoppedState(void)
{
  digitalWrite(OUT_DO_RUN, LOW);
  digitalWrite(OUT_DO_SWITCH, LOW);
  digitalWrite(OUT_DIODE_YELLOW_OPEN, HIGH);
  digitalWrite(OUT_DIODE_YELLOW_CLOSE, HIGH);

  delay(SWITCH_DELAY_TIME);

  if (IS_CLOSING == stateBeforeStopped)
  {
    doorState = IS_OPENING;
  }
  else
  {
    doorState = IS_CLOSING;
  }
}

void handleErrorState(void)
{
  digitalWrite(OUT_DO_RUN, LOW);
  digitalWrite(OUT_DO_SWITCH, LOW);
  digitalWrite(OUT_DIODE_YELLOW_OPEN, LOW);
  digitalWrite(OUT_DIODE_YELLOW_CLOSE, LOW);

  digitalWrite(OUT_DIODE_RED_ERROR, HIGH);
  delay(200);
  digitalWrite(OUT_DIODE_RED_ERROR, LOW);
  delay(200);
}

void setup() {
  pinMode(IN_TASTER_UP, INPUT);
  pinMode(IN_TASTER_DOWN, INPUT);
  pinMode(IN_SWITCH_UP, INPUT);
  pinMode(IN_SWITCH_DOWN, INPUT);

  digitalWrite(IN_TASTER_UP, HIGH);
  digitalWrite(IN_TASTER_DOWN, HIGH);
  digitalWrite(IN_SWITCH_UP, HIGH);
  digitalWrite(IN_SWITCH_DOWN, HIGH);
  
  pinMode(OUT_DO_RUN, OUTPUT);
  pinMode(OUT_DO_SWITCH, OUTPUT);
  pinMode(OUT_DIODE_YELLOW_OPEN, OUTPUT);
  pinMode(OUT_DIODE_YELLOW_CLOSE, OUTPUT);
  pinMode(OUT_DIODE_RED_ERROR, OUTPUT);

  //Serial.begin(9600);
}

void loop() {
  readInputs();
  checkErrorCondition();

  switch (doorState)
  {
    case OPENED:
    {
      if (LOW == inTasterDownState)
      {
        doorState = IS_CLOSING;
        time_reset = millis();
      }
      else
      {
        handleOpenedState();
      }
      break;
    }
    case IS_CLOSING:
    {
      if (false != errorConditionSatisfied)
      {
        doorState = ERROR_STATE;
        break;
      }      
      if (HIGH == inSwitchDownState)
      {
        doorState = CLOSED;
        break;
      }      
      if (LOW == inTasterUpState)
      {        
        stateBeforeStopped = IS_CLOSING;

        doorState = DELAYED;
      }
      else
      {
        handleIsClosingState();
      }      
      break;
    }
    case CLOSED:
    {
      if (LOW == inTasterUpState)
      {
        doorState = IS_OPENING;
        time_reset = millis();
      }
      else
      {
        handleClosedState();
      }
      break;
    }
    case IS_OPENING:
    {
      if (false != errorConditionSatisfied)
      {
        doorState = ERROR_STATE;
        break;
      }
      if (HIGH == inSwitchUpState)
      {
        doorState = OPENED;
        break;
      }
      if (LOW == inTasterDownState)
      {        
        stateBeforeStopped = IS_OPENING;

        /* Stop before going to IS_CLOSED state. */
        doorState = DELAYED;
      }
      else
      {
        handleIsOpeningState();
      }
      break;
    }
    case DELAYED:
    {
      // if ()
      // {
      //   doorState = IS_OPENING;
      //   break;
      // }
      // if ()
      // {
      //   doorState = IS_CLOSING;
      //   break;
      // }

      handleStoppedState();
      break;
    }
    case ERROR_STATE:
    {
      handleErrorState();
      break;
    }
    default:
    {
      /* Arduino HW failure, probably reset the controller. */
      handleErrorState();
    }
  }
}
