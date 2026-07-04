#include "State.hpp"

State::State() : keys(SDL_GetKeyboardState(nullptr))
{
}

State::~State()
{
    SDL_DestroyWindow(_window);
    SDL_DestroyRenderer(_renderer);
}

void State::init()
{
    _window = SDL_CreateWindow("snake", width, height, SDL_WINDOW_RESIZABLE);
    _renderer = SDL_CreateRenderer(_window, nullptr);
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
}