const unsigned int BIT_LENGTH = 50; // millis per bit
const int clients = 2; // Clients in ring
const unsigned char bytesPerClient = 2;
const unsigned char message_bytes = bytesPerClient * clients + 2; 
const unsigned char FRAME_BYTES =  message_bytes + 2; 
const unsigned char BUS_PIN = 7; 
const unsigned char clientID = 1;
const unsigned char destinationID = 2;
struct State {
  unsigned int state_millis; // Removed long int! If program breaks reimplement
  unsigned char state_value;
};
struct Payload {
char payloadContent[8];
int payloadPosition = 0;
bool payloadDone = false;
};

void setup() {
  Serial.begin(9600);
  pinMode(BUS_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  static unsigned char token[message_bytes]; // token which is passed around
  static char message[8]; // Message which will be sent to other client
  static bool tokenHolder = true; // Checks if client is current tokenholder
  static bool messageEntered = false; // Checks if message has been input

  if(!tokenHolder){
    reciveMessage(token, &tokenHolder, message, &messageEntered);
  }else {
    delay(500);
    Serial.print("Token: ");
    for(int i = 0; i < 8; i++) {
      char t= token[i];
      Serial.print(t);
    }
    Serial.println();
    sendMessage(token);
    tokenHolder = false;
  }

  // Input 
  static int currentMessageIndex = 0;
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

// Outputs the token to a pin
void sendMessage(unsigned char * token) {
  unsigned char raw_message[FRAME_BYTES];
  raw_message[0] = (1<<0)|(1<<1)|(1<<3)|(1<<5)|(1<<7);
  raw_message[FRAME_BYTES] = 0;
  for (int index = 1; index < FRAME_BYTES - 1; index++)
  {
    raw_message[index] = token[index - 1];
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
    TIMSK1 = 0;
  }
  if (Serial) {
    Serial.println(messageString);
    Serial.println(messageString.length());
    Serial.print("frame delta: ");
    Serial.println(current_millis - start_millis);
  }
  pinMode(BUS_PIN, INPUT);
}

// Checks if there is a token being sent and captures it
void reciveMessage (unsigned char * token, bool * tokenHolder, char * message, bool * messageEntered) {
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
        token[index] |= (states[8 + i + (index * 8)].state_value<<(7 - i));
      }  
    }
    word_done = true;
  }

  if (word_done) {
    word_start = 0;
    word_started = false;
    last_state = 0;
    word_done = false;
    state_count = 0;
    // Checks if client is recipient of token
    if(token[1] == clientID) {
      *tokenHolder = true;
      readToken(token);
      updateToken(token, message, &messageEntered);
    }
  }
}

// Updates source address, destination address, and payload of token if there is one
void updateToken(unsigned char * token, char * message, bool ** messageEntered) {
  static int currentMessageIndex = 0;
  if (**messageEntered)
  {
    token[clientID + 2] = message[currentMessageIndex];
    message[currentMessageIndex] = 0;
    currentMessageIndex++;
  } else {
    token[clientID + 2] = 0;
  }
  if (currentMessageIndex > 7 || message[currentMessageIndex] == 0)
  {
    **messageEntered = false;
  }
  
  token[0] = clientID;
  if (clientID == clients)
  {
    token[1] = 1;
  } else
  {
    token[1] = clientID + 1;
  }
  token[clientID + 1] = destinationID;
}

// Reads the content of a token and stores recived message until complete message has been recived. Then prints it out
bool readToken(unsigned char * token) {
  static Payload payloads[clients];

  if (token[1] == clientID)
  {
    for (int index = 0; index < clients; index++)
    {
      if (!(clientID - 1 == index))
      {
        if (token[2 + (index * 2)] == clientID)
        {
          char currentPayload = token[3 + (index * 2)];
          token[3 + (index * 2)]= 0;
          if(currentPayload == 0 || payloads[index].payloadPosition > 7 && payloads[index].payloadDone == false) {
            payloads[index].payloadDone = true;
            Serial.print("Recived message: ");
            for (int currentChar = 0; currentChar < 8; currentChar++)
            {
              Serial.print(payloads[index].payloadContent[currentChar]);
              payloads[index].payloadContent[currentChar] = 0;
            }
            Serial.println();
            payloads[index].payloadPosition = 0;
          } else{
            if (currentPayload == !0)
            {
              payloads[index].payloadDone = false;
            }
            payloads[index].payloadContent[payloads[index].payloadPosition] = currentPayload;
            payloads[index].payloadPosition++;
          }
        } else if (payloads[index].payloadDone == false)
        {
          payloads[index].payloadDone = true;
          Serial.print("Recived message: ");
          for (int currentChar = 0; currentChar < 8; currentChar++)
          {
            Serial.print(payloads[index].payloadContent[currentChar]);
            payloads[index].payloadContent[currentChar] = 0;
          }
          Serial.println();
          payloads[index].payloadPosition = 0;
        }       
      }
    }  
  } 
}
