#include "State.hpp"

State::State() : keys(SDL_GetKeyboardState(nullptr))
{
}

State::~State()
{
    SDL_DestroyWindow(_window);
    _window = nullptr;
    SDL_DestroyRenderer(_renderer);
    _renderer = nullptr;
    MIX_DestroyMixer(_mixer);
    _mixer = nullptr;
    TTF_DestroyRendererTextEngine(_engine);
    _engine = nullptr;
}

void State::init()
{
    _window = SDL_CreateWindow("snake", width, height, SDL_WINDOW_RESIZABLE);
    _renderer = SDL_CreateRenderer(_window, nullptr);
    _mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    _engine = TTF_CreateRendererTextEngine(_renderer);
    SDL_SetRenderLogicalPresentation(_renderer, logW, logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_RenderCoordinatesFromWindow(_renderer, width, height, &logX, &logY);
    check_init();
}

void State::check_init()
{
    if (!_window)
    {
        this->~State();
        throw "SDL: create window failed\n";
    }
    if (!_renderer)
    {
        this->~State();
        throw "SDL: create renderer failed\n";
    }
    if (!_mixer)
    {
        this->~State();
        throw "SDL: create mixer failed\n";
    }
    if (!_engine)
    {
        this->~State();
        throw "SDL: create text_engine failed\n";

    }
}