Change recognition model:
1. Copy the packed model file(pack with text, XXX_packed_WithTxt.bin) to src/Model.
2. Modify the line 8 of src/CybModel.asm: .incbin "../src/Model/XXX_pack_withTxt.bin"
3. Please use XXX_packed_withTxtwithTri.bin if need to support voice tag.


There are some compile options defined in voice_main.h for different purpose:

1. #define RECOG_FLOW               RECOG_FLOW_NORMAL
   We treat the first group as trigger words and the second group(if it exist) as command
   words if it exist.
   At first stage, we only recognize the trigger word. After trigger word recognized, we
   recognize the command words at the second stage. If no command detected in 6 seconds
   (defined by COMMAND_RECOGNIZE_TIME_MIN), it will switch back to trigger mode. The max
   wait time is defined by COMMAND_RECOGNIZE_TIME_MAX.
   We define trigger word in the group 0(GROUP_INDEX_TRIGGER) model, and command words in
   the group 1(GROUP_INDEX_COMMAND) model.
   
2. #define RECOG_FLOW               RECOG_FLOW_NONE
   Record only, no recognition.
   
3. #define SUPPORT_SPEEX_PLAY
   After we get the recognition result, we will play the response voice. The response voice
   is compressed by Speex codec. We pack many Speec compressed data files to one binary file.
   Every data file may set a ID, and we can play it by specify the ID.

4. #define SUPPORT_RECOG_TWO_MODEL
   We support to recognize two language model at the same time. the CPU/RAM/Model_ROM usage 
   will become double.

5. #define SUPPORT_VOICE_TAG
   To record voice tag, please click SW2. Please say voice tag in 5 seconds, then it will 
   switch back to recognition mode. If we record voice tag successfully, it can be recognized
   both in the two stage.
   This sample code store the voice tag model in g_byaSDModel[DSPOTTER_SD_MODEL_SIZE]. The 
   voice tag model contain 300 bytes header and 100~300 bytes per voice tag.  
   For training the voice tag model, we change the DSpotter memory buffer from 50KB to 75KB.
   We also declare extra two array to store voice(12KB) and model(2KB). 
