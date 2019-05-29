const unsigned long int BIT_LENGTH = 50; // millis per bit
const int clients = 2; // Clients in ring
const unsigned char bytesPerClient = 2;
const unsigned char message_bytes = bytesPerClient * clients + 2; // 
const unsigned char FRAME_BYTES =  message_bytes + 2; 
const unsigned char BUS_PIN = 7; 
const int clientID = 1;
unsigned char token[message_bytes];
struct Payload
{
  char payloadContent[8];
  int payloadPosition = 0;
  bool payloadDone;
};
Payload payloads[clients];
unsigned char * tokenPointer;
bool tokenHolder = true;

void setup() {
  Serial.begin(9600);
  pinMode(BUS_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  tokenPointer = token;
}

void loop() {
  if (tokenHolder)
  {
    unsigned char destinationID = 2;
    unsigned char payload = 'A';
    updateToken(token, message_bytes, clientID, destinationID, payload);
  } else
  {
    readToken(token, clients, clientID);
  }
}

void updateToken(unsigned char * token, unsigned char message_bytes, unsigned char clientID, unsigned char destinationID, unsigned char payload) {
  * token = clientID;
  if (clientID == clients)
  {
    * (token + 1) = 1;
  } else
  {
    * (token + 1) = clientID + 1;
  }
  * (token + clientID + 1) = destinationID;
  * (token + clientID + 2) = payload;
}

bool readToken(unsigned char * token, int clients, unsigned char clientID) {
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

void sendMessage(unsigned char * token) {
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
