#define HASH_SIZE 32

#include <SHA256.h>

void print_uint64_t(uint64_t num);

struct Block {
  uint64_t index;
  uint64_t proof_number;
  // fixed len = HASH_SIZE
  uint8_t* prev_hash;
  uint8_t* data;
  size_t data_len;
  uint64_t timestamp;

  Block (
    uint64_t _index,
    uint64_t _proof_number,
    uint8_t* _prev_hash,
    uint8_t* _data,
    size_t _data_len,
    uint64_t _timestamp = millis())
    : index(_index),
      proof_number(_proof_number),
      //    prev_hash(_prev_hash),
      //    data(data),
      data_len(_data_len),
      timestamp(_timestamp) {
    data = malloc(data_len * sizeof(uint8_t));
    for (int i = 0; i < data_len; ++i) {
      data[i] = _data[i];
    }
    prev_hash = malloc(HASH_SIZE * sizeof(uint8_t));
    for (int i = 0; i < HASH_SIZE; ++i) {
      prev_hash[i] = _prev_hash[i];
    }
  }


  uint8_t* calculate_hash() {
    SHA256 sha256;
    sha256.update(index, 8);
    sha256.update(proof_number, 8);
    sha256.update(prev_hash, HASH_SIZE);
    sha256.update(data, data_len);
    sha256.update(timestamp, 8);

    uint8_t result[HASH_SIZE];
    sha256.finalize(result, HASH_SIZE);

    return result;
  }

  void print() {
    Serial.print("index: ");
    print_uint64_t(index);
    Serial.print(", proof_number: ");
    print_uint64_t(proof_number);
    Serial.print(", previous hash: ");
    for (int i = 0; i < HASH_SIZE; ++i) {
      Serial.print(prev_hash[i], HEX);
    }
    Serial.print(", data: ");
    for (int i = 0; i < data_len; ++i) {
      Serial.print(data[i], HEX);
    }
    if (data_len == 0) {
      Serial.print("[]");
    }
    Serial.print(", timestamp: ");
    print_uint64_t(timestamp);
    Serial.println("");
  }
};

struct BlockChain {
  Block* chain;
  size_t chain_len;
  uint8_t* current_data;
  size_t current_data_len;

  BlockChain() {
    chain = malloc(0);
    chain_len = 0;
    current_data = malloc(0);
    current_data_len = 0;

    genesis();
  }

  void genesis() {
    uint8_t* prev_hash = malloc(HASH_SIZE * sizeof(uint8_t));
    for (int i = 0; i < HASH_SIZE; ++i) {
      prev_hash[i] = 0;
    }
    constructBlock(0, prev_hash);
    free (prev_hash);
  }

  Block* constructBlock(uint64_t proof_number, uint8_t* prev_hash) {
    Block* block = new Block(chain_len, proof_number, prev_hash, current_data, current_data_len);
    free(current_data);
    current_data = malloc(0);
    current_data_len = 0;
    Serial.println("Appending block");
    appendBlock(block);
    return block;
  }

  void appendBlock(Block* block) {
    Block* reallocatedChain = realloc(chain, ++chain_len * sizeof(Block));
    Serial.print("Realocated chain nillnes: ");
    Serial.println(reallocatedChain != NULL);
    // TODO: possible memory loss (!)
    if (reallocatedChain != NULL) {
      chain = reallocatedChain;
      chain[chain_len - 1] = *block;
    } else {
      Serial.println("Unable to reallocate memory for chain");
    }
  }

  bool compareArrays(uint8_t* arr1, uint8_t* arr2, size_t len) {
    for (int i = 0; i < len; ++i) {
      if (arr1[i] != arr2[i]) {
        return false;
      }
    }

    return true;
  }

  bool checkValidity(Block* block, Block* prev_block) {
    if (block->index + 1 != prev_block->index) {
      return false;
    } else if (!compareArrays(prev_block->calculate_hash(), block->prev_hash, HASH_SIZE)) {
      return false;
    } else if (!verifyingProof(block->proof_number, prev_block->proof_number)) {
      return false;
    } else if (block->timestamp <= prev_block->timestamp) {
      return false;
    }
    return true;
  }

  void newData(char* sender, size_t sender_len, char* recipient, size_t recipient_len, uint64_t quantity) {
    uint8_t* reallocated_data = realloc(current_data, (sender_len + recipient_len) * sizeof(char) + sizeof(uint64_t));
    // TODO: possible memory loss
    if (reallocated_data) {
      current_data = reallocated_data;
      for (int i = current_data_len; i < sender_len + current_data_len; ++i) {
        current_data[i] = sender[sender_len + current_data_len - i];
      }
      current_data_len += sender_len;
      for (int i = current_data_len; i < recipient_len + current_data_len; ++i) {
        current_data[i] = recipient[recipient_len + current_data_len - i];
      }
      current_data_len += recipient_len;
      uint8_t quantity_arr[sizeof(uint64_t)];
      memcpy(quantity_arr, quantity, sizeof(uint64_t));
      for (int i = current_data_len; i < sizeof(uint64_t) + current_data_len; ++i) {
        current_data[i] = quantity_arr[sizeof(uint64_t) + current_data_len - i];
      }
      current_data_len += sizeof(uint64_t);
    } else {
      Serial.println("Unable to reallocate to append new data");
    }
  }

  uint64_t proofOfWork(uint64_t last_proof_number) {
    uint64_t proof_number = 0;

    while (!verifyingProof (proof_number, last_proof_number)) {
      ++proof_number;
    }

    return proof_number;
  }

  bool verifyingProof(uint64_t last_proof_number, uint64_t proof_number) {
    SHA256 sha256;
    sha256.update(last_proof_number, sizeof(uint64_t));
    sha256.update(proof_number, sizeof(uint64_t));

    uint8_t result[HASH_SIZE];
    sha256.finalize(result, HASH_SIZE);
    
    for (int i = 0; i < HASH_SIZE; ++i) {
      Serial.print(result[i], HEX);
    }
    Serial.println("");
//    Serial.println(result[0] == '0');
    
    return result[0] == 0x8c && result[1] == 0x8c;  
//    return result[0] == 0 && result[1] == 0 && result[2] == 0 && result[3] == 0;
  }

  Block* getLatestBlock() {
    return &chain[chain_len - 1];
  }

  void blockMining  (char* details_miner, uint64_t details_miner_len) {
    newData("0", 1, details_miner, details_miner_len, 1);

    Block* last_block = getLatestBlock();
    uint64_t last_proof_number = last_block->proof_number;

    uint64_t proof_number = proofOfWork(last_proof_number);

    Serial.print("Found proof number: ");
    print_uint64_t(proof_number);
    Serial.println("");

    char* last_hash = last_block->calculate_hash();
    Serial.print("Calculated hash: ");
    for (int i = 0; i < HASH_SIZE; ++i) {
      Serial.print(last_hash[i], HEX);
    }
    Serial.println("");

    Block* block = constructBlock(proof_number, last_hash);

    Serial.print("Nillness check: ");
    Serial.print(last_hash == NULL);
    Serial.print(last_block == NULL);
    Serial.print(block == NULL);
    Serial.println("");
//    if (last_hash != NULL){
//      free(last_hash);
//    }
////    free(last_hash);
//    free(last_block);
//    free(block);
  }

  void print() {
    Serial.println("Printing out all blocks:");
    for (int i = 0; i < chain_len; ++i) {
      chain[i].print();
    }
  }
};

BlockChain* blockchain;

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting mining");
  blockchain = new BlockChain();
  blockchain-> print();

//  Block* last_block = blockchain->getLatestBlock();
//  uint64_t last_proof_number = last_block->proof_number;
//  uint64_t proof_number = blockchain->proofOfWork(last_proof_number);
  blockchain->blockMining("Quircy Larson", 13);
  Serial.println("Mining was successful");
  blockchain->print();

}

void loop()
{
}

void print_uint64_t(uint64_t num) {
  if (num == 0) {
    Serial.print(0);
    return;
  }

  char rev[128];
  char *p = rev + 1;

  while (num > 0) {
    *p++ = '0' + ( num % 10);
    num /= 10;
  }
  p--;
  /*Print the number which is now in reverse*/
  while (p > rev) {
    Serial.print(*p--);
  }
}
