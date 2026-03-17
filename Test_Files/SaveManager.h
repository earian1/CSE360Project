#pragma
#include <string>
#include <fstream>

class SaveManager {

    public:
    
        bool saveToFile(const std::string& filePath)
        {
            std::ofstream file(filePath, std::ios::binary);
            if (!file.is_open())
                return false;

            const char Header[44] = {
                'R','I','F','F',  // ChunkID
            36,0,0,0,         // ChunkSize (36 + 0 bytes of data)
            'W','A','V','E',  // Format
            'f','m','t',' ',  // Subchunk1ID
            16,0,0,0,         // Subchunk1Size (PCM)
            1,0,              // AudioFormat (PCM=1)
            1,0,              // NumChannels (mono)
            68,-84,0,0,       // SampleRate (44100)
            68,-84,0,0,       // ByteRate
            2,0,              // BlockAlign
            16,0,             // BitsPerSample
            'd','a','t','a',  // Subchunk2ID
            0,0,0,0           // Subchunk2Size (0 bytes of audio)
        };

            file.write(Header, 44);

            lastSavedPath = filePath;
            return file.good();
        }

        bool fileExists(const std::string& filePath) const
        {
            std::ifstream file(filePath);
            return file.good();
        }

        std::string getLastSavedPath() const
        {
            return lastSavedPath;
        }

    private:
        std::string lastSavedPath;
};