const unsigned long int BIT_LENGTH = 50; // millis per bit
const int clients = 2; // Clients in ring
const unsigned char bytesPerClient = 2;
const unsigned char message_bytes = bytesPerClient * clients + 2; // 
const unsigned char FRAME_BYTES =  message_bytes + 2; 
const unsigned char BUS_PIN = 7; 
const unsigned char clientID = 1;
// unsigned char token[message_bytes];
char message[8];
int currentMessageIndex = 0;
bool messageEntered = false;
bool messageTimer = false;
struct Payload {
  char payloadContent[8];
  int payloadPosition = 0;
  bool payloadDone = false;
};
Payload payloads[clients];
unsigned char * token[message_bytes];
bool tokenHolder = true;
struct State {
  unsigned long int state_millis;
  unsigned char state_value;
};

void setup() {
  Serial.begin(9600);
  pinMode(BUS_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  // * tokenPointer = token;

  //  Config Timer:
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536) 1 sec = 15624; 0,5s = 7812
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}


void loop() {
  volatile static unsigned char payload = 0;
  if(messageTimer) {
    Serial.print("Token: ");
    for(int i = 0; i < 8; i++) {
      char t= (token + i);
      Serial.print(t);
    }
    Serial.println();
    sendMessage(token);
    messageTimer = false;
    tokenHolder = true;
  }
  if (tokenHolder)
  {
    static unsigned char destinationID = 2;
    if(messageEntered){
      payload = message[currentMessageIndex];
      currentMessageIndex++;
      if(currentMessageIndex >= 8) {
        currentMessageIndex = 0;
        messageEntered = false;
        for (int index = 0; index < 8; index++)
        {
          message[index] = 0;
        }
      }
    }
    updateToken(destinationID, payload);
    tokenHolder = false;
    // Serial.println("Token changed");
  } else
  {
    reciveMessage();
    // Serial.println("Message read");    
  }

  if (!messageEntered)
  { 
    char input = Serial.read();
    if (currentMessageIndex >= 8)
    {
      Serial.print("input: ");
      for(int i = 0; i < 8; i++){
        Serial.print(message[i]);
      }
      Serial.println();
      currentMessageIndex = 0;
      messageEntered = true;
    } else
    {
      if (input == '\n')
      {
        Serial.print("input: ");
        for(int i = 0; i < 8; i++){
          Serial.print(message[i]);
        }
        Serial.println();
        currentMessageIndex = 0;
        messageEntered = true;
      } else if(input >= 0 && input <= 127)
      {
        message[currentMessageIndex] = input;
        Serial.println("Character entered");
        currentMessageIndex++;
      }
    }
  }
}


void updateToken(unsigned char destinationID, unsigned char payload) {
  token[0] = &clientID;
  if (clientID == clients)
  {
    token[1] = 1;
  } else
  {
    token[1] = clientID + 1;
  }
  token[clientID + 1] = &destinationID;
  token[clientID + 2] = &payload;
}

bool readToken() {
  if (* (token + 1) == clientID)
  {
    for (int index = 0; index < clients; index++)
    {
      if (!(clientID - 1 == index))
      {
        if ((* (token + 2 + (index * 2)) == clientID) && payloads[index].payloadDone == true)
        {
          payloads[index].payloadContent[payloads[index].payloadPosition] = * (token + 3 + (index * 2));
          payloads[index].payloadPosition++;
          payloads[index].payloadDone = false;
          * (token + 3 + (index * 2)) = 0;
        } else if (payloads[index].payloadDone == false)
        {
          payloads[index].payloadDone = true;
          for (int i = payloads[index].payloadPosition; i < 8; i++)
          {
            payloads[index].payloadContent[i] = 0;
          }
          payloads[index].payloadPosition = 0;
        }
        
      }
    } 
    tokenHolder = true; 
  } 
}

void printPayloads() {
  for (int index = 0; index < clients; index++)
  {
    if (payloads[index].payloadDone == true)
    {
      Serial.print("Message from client ");
      Serial.print(index + 1);
      Serial.print(": ");
      for (int i = 0; i < 8; i++)
      {
        Serial.print(payloads[index].payloadContent[i]);
      }
      Serial.println();
      payloads[index].payloadDone = false;
    }   
  } 
}

void sendMessage(unsigned char * token[]) {
  unsigned char raw_message[FRAME_BYTES];
  raw_message[0] = (1<<0)|(1<<1)|(1<<3)|(1<<5)|(1<<7);
  raw_message[FRAME_BYTES] = 0;
  for (int index = 1; index < FRAME_BYTES - 1; index++)
  {
    raw_message[index] = * (token + index - 1);
    raw_message[FRAME_BYTES] = raw_message[FRAME_BYTES] ^ raw_message[index];
  }
  unsigned long int start_millis = millis();
  unsigned long int current_millis = millis();
  pinMode(BUS_PIN, OUTPUT);
  unsigned long int frame_duration = FRAME_BYTES * 8 * BIT_LENGTH;
  if (Serial) {
    Serial.print("Frameduration: ");
    Serial.println(frame_duration);
  }
  unsigned char last_bit = 3;
  String messageString = "";
  unsigned char last_bit_count = 9;
  while ( (current_millis - start_millis) < frame_duration) {
    unsigned char bit_count = (current_millis - start_millis) / BIT_LENGTH;
    unsigned char byte_count = bit_count / 8;
    unsigned char current_bit_number = 7 - bit_count % 8;
    unsigned char current_bit = raw_message[byte_count] & (1 << current_bit_number);
    if ( bit_count != last_bit_count) {
      last_bit_count=  bit_count;    
      messageString += current_bit ? "_" : ".";
    }
    if (last_bit != current_bit) {
      digitalWrite(BUS_PIN, current_bit);
      digitalWrite(LED_BUILTIN, current_bit);
      last_bit = current_bit;
    }
    current_millis = millis();
  }
  if (Serial) {
    Serial.println(messageString);
    Serial.println(messageString.length());
    Serial.print("frame delta: ");
    Serial.println(current_millis - start_millis);
  }
  pinMode(BUS_PIN, INPUT);
}

void reciveMessage () {
  volatile static State states[256];
  volatile static unsigned long word_start = 0;
  volatile static unsigned char word_started = false;
  volatile static unsigned char last_state = 0;
  volatile static unsigned char word_done = false;
  volatile static unsigned char state_count = 0;

  volatile unsigned long current_millis = millis();
  volatile unsigned char current_state = digitalRead(BUS_PIN);

  if (last_state != current_state) {
    last_state = current_state;
    digitalWrite(LED_BUILTIN, current_state);
    if (!word_started) {
      word_start = current_millis;
      word_started = true;
    }
    states[state_count].state_millis = current_millis;
    states[state_count].state_value = current_state;
    state_count++;
  }

  if ( word_started && (current_millis - word_start) > BIT_LENGTH * FRAME_BYTES * 8) {
    Serial.print("Got a Frame ");
    Serial.print(" ");
    Serial.print(" ");
    Serial.println(state_count);
    for (int bit_count = 0; bit_count < FRAME_BYTES * 8; bit_count ++) {
      bool current_bit = states[0].state_value;
      for (int state_count = 0;
           state_count < FRAME_BYTES * 8;
           state_count ++) {
        if ((states[state_count].state_millis - states[0].state_millis) <
            (bit_count * BIT_LENGTH + (BIT_LENGTH / 2) ) ) {
          current_bit = states[state_count].state_value;
        }
      }
      Serial.print(current_bit);
    }
    Serial.println();
    for (int index = 0; index < message_bytes; index++)
    {
      for (int i = 0; i < 8; i++)
      {
        token[index] |= (unsigned char)(states[8 + i + (index * 8)].state_value<<(7 - i));
      }  
    }
    word_done = true;
    readToken();
  }

  if (word_done) {
    word_start = 0;
    word_started = false;
    last_state = 0;
    word_done = false;
    state_count = 0;
    TIMSK1 |= (1 << OCIE1A);
  }
}

ISR(TIMER1_COMPA_vect){
  TCNT1  = 0;//initialize counter value to 0
  TIMSK1 = 0;
  messageTimer = true;
  Serial.println("Interupt");
}
