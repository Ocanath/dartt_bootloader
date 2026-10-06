# Secure Bootloader Extension: Design Plan

Status: draft, not implemented. Nothing in this document is in the code yet.

---

The current implementation of DARTT bootloader is completely open and in the clear. It provides a memory manipulation interface for microcontrollers with write, read, and erase primitives, integrity checks via CRC32, and basic memory protections (the bootloader can't overwrite itself directly, memory boundaries are enforced within the DARTT API). This is fine for personal use and development, but for any kind of deployment in an untrusted environment (especially if exposed over a shared network, such as over a TCP socket) it's unacceptable, as anyone can read, erase and overwrite your devices firmware as long as they know the bootloader access mechanism and command structure.

The goal of this project is to build a secure extension to the DARTT bootloader, to authenticate and obfuscate DARTT bootloader operations. This locks the interface down so that all sensitive information (command words, read/write contents) is encrypted, and every sensitive action (read, write, erase, start application, etc) is authenticated. In the process, a core design strategy will be developed around secure communications over DARTT in general, involving key cryptographic primitives (shared keys, session keys, authentication tags, nonces).

This is the first project involving cryptography over DARTT. DARTT is designed to have extremely low overhead with flexible packet sizes to maximize speed. For example, over a good RS485 connection, a 128 byte payload requires only 7 bytes of overhead (address, index, CRC16, COBS). This provides low enough overhead for low latency communications (e.g. 6 bytes of encoded motion data at >1khz update rate), while also supporting higher latency, high throughput communications (such as [audio over dartt](https://github.com/ocanath/dartt_audio)). 

**DISCLAIMER:** This is the first exercise in building a secure system that I have ever undertaken. I will probably fuck something up. Unless this project somehow magically starts getting external review and PR's, take it for what it is: a solo C developer's first attempt at baking security into a project. 

Design Plan
---

The first key design decision in this project is where and how to exchange cryptographic primitives, such as ciphertext, nonces, MAC's, and keys. In my current view, there are two options: 

1. Bake cryptographic primitives into the DARTT frame structure itself: add nonce and MAC, encrypt the index and data fields, while keeping the existing device addressing scheme, frame delimiter logic (COBS), and data integrity check (CRC). 

2. Implement cryptographic primitives into a secure DARTT register map. Nonce, MAC and ciphertext each go to pre-determined DARTT memory regions (e.g. working buffers). The map would also contain useful clear primtives such as a length field and a 'run' register that gates processing of a complete cryptographic message. Information that is accessed in the clear, such as nonces and public keys, exist in read-only fields of a DARTT map and are accessed using the unmodified API. This means crypto is *application defined*. 

Method 2. will be used for this bootloader, for the following reasons.

Authenticated crypto requires a large amount of overhead. Message authentication codes (MAC) are anywhere from 4-512bytes in size, nonces are 8-24bytes. The current plan for this project will use a 24byte nonce and 16 byte mac. Secure systems also often require a combination of information which is exchanged in the clear (e.g. public keys, nonces) as well as encrypted information. Managing this additional information in a DARTT register map is advantageous because it uses the existing DARTT API with no new provisions to cleanly differentiate between clear and encrypted content. It also maintains the efficiency of DARTT - reading a 32 byte public key will still have only a few bytes of overhead.

Another important aspect of method 2. is *fragmentation*. The DARTT frame format is inherently suited to fragmentation, and the API supports fragmentation via  `_write_multi()` and `_read_multi()`. This is especially important if implementing cryptography over standard CAN, which has 8 byte maximum payload sizes - in such a case, fragmentation is necessary because it is simply not possible to contain cryptographic primitives in a single message. 

The primary risk with fragmentation is DOS. Proper implementation of the DARTT memory map can mitigate this, however - for example, a secure system with a high likelihood of an attacker sending phony fragments as a DOS strategy could add an extension to the DARTT API that requires auth for each write frame, enforced at parse time - for example, you can embed a nonce, MIC/MAC, and DARTT message containing a data fragment within a parent DARTT message which writes to working buffer prepended with the nonce and MAC. At parse time the reciever requires authentication of the entire frame via MAC and tosses the whole thing out if it fails. This technique is still not possible for standard CAN but is viable for FDCAN, which supports 64byte message sizes.

For standard CAN, some DOS risk must be accepted - don't use standard CAN if you want authentication, encryption, and DOS resistance.


### Encryption Library:

The encryption library selected for this project is [monocypher](https://monocypher.org/), a popular, embedded friendly C library implementing cryptographic algorithms and primitives that is easy to link. It was selected because it's easy to link and integrate in the current project structure, has good documentation, has a healthy open source community of testers and developers, is popular (satisfying the golden rule of 'don't roll your own crypto'), and is embedded-friendly. 

### Memory Map:

The bootloader structure will be reorganized, as follows:

| Name | Word |
|--| -- |
| Status Register | 0 |
| Nonce | 1-6 |
| Working Buffer (ciphertext) | 7-22|
| Action Register (ciphertext) | 23 |
| Message Authentication Code | 24-27 |
| Working size | 28 |
| Additional cleartext parameters... |28 - N|

The MAC is calculated based on the nonce, working size, and ciphertext contents. The ciphertext contains the contents of the working buffer and the action register contents. 

The erase semantics, which have their own dedicated registers, will be removed from the map and replaced with contextual positional loading of the working buffer. For example, `erase_page` and `erase_num_pages` behavior can be loaded in bytes 0-3 and 4-7 of the working buffer, and extracted at parse-time.

The working size register will gate processing of the secure region. I.e. once the working size changes from 0 to a nonzero value, the entire secure region (not including the status register) will be read. This creates DOS vulnerability - i.e. someone can come in and modify a chunk of the message to cause it to be tossed out, or spam working size to prevent any data from getting processed. If DOS sensitivity is a concern the bootloader should be modified to require all 112 bytes to be sent in one datagram. 

The bootloader's persistent settings should also be protected by authentication. 

### Session Key (SK):

Each host-peripheral session will derive session keys based on a random, shared input `R`, which will exist in the register map. The exact algorithm to derived session keys is not yet decided - more research is required. 

## System Flow:

The general system flow ideally will be something like this:

1. Peripheral init. Create a new, non-repeating, cleartext random word R and place it in the DARTT map, enforcing read-only behavior. 
1. Peripheral uses R to create session keys (host->device, device->host).
1. Host init. Host reads R and generates session keys (host->device, device->host).




