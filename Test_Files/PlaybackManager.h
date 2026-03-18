#pragma

class PlaybackManager
{
    public:
        enum class State {
            Stopped,
            Playing,
            Paused
        };

        void play() {
            if (audioLoaded && (currentState == State::Stopped || currentState == State::Paused)) 
                currentState = State::Playing;
            
        }

        void pause() {
            if (currentState == State::Playing)
                currentState = State::Paused;
        }

        void resume() {
            if (currentState == State::Paused)
                currentState = State::Playing;
        }

        void stop() {
            currentState = State::Stopped;
        }

        void loadAudio(bool loaded) {
            audioLoaded = loaded;
            
        }

        State getState() const {
            return currentState;
        }

        bool isPlaying() const {
            return currentState == State::Playing;
        }

        bool isPaused() const {
            return currentState == State::Paused;
        }

        bool isStopped() const {
            return currentState == State::Stopped;
        }

        
        private:
        State currentState { State::Stopped };
        bool audioLoaded { false };
};