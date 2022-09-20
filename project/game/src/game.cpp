#include <SFML/Graphics.hpp>

#include "nova/nova.hpp"

using namespace nova;

struct velocity {
  float dy{};
  float dx{};
};

auto speed_system(Resource<const Time> time,
                  View<With<const velocity, sf::CircleShape>> view) {
  for (auto&& [_, velocity, circle] : view.each()) {
    const auto [x, y] = circle.getPosition();

    const auto delta = time->delta_seconds<float>();
    const auto new_x = x + velocity.dx * delta;
    const auto new_y = y + velocity.dy * delta;
    circle.setPosition(sf::Vector2f{new_x, new_y});
  }
}

auto spawn_circle(Registry& registry) {
  const auto e = registry.create();
  registry.emplace<velocity>(e, 10.f, 10.f);

  auto circle = sf::CircleShape();
  circle.setRadius(10.f);
  circle.setFillColor(sf::Color::Blue);
  registry.emplace<sf::CircleShape>(e, circle);
}

auto draw_circle(Resource<sf::RenderWindow> window,
                 View<With<const sf::CircleShape>> view) {
  window->clear();
  for (auto&& [_, circle] : view.each()) {
    window->draw(circle);
  }
  window->display();
}

auto exit_game(Resource<sf::RenderWindow> window, Resource<AppExit> exit) {
  sf::Event event{};
  while (window->pollEvent(event)) {
    if (event.type == sf::Event::Closed) {
      exit->should_exit = true;
      window->close();
    }
  }
}

int main() {
  auto app = App{};
  app.add_plugin(DefaultPlugins{})
      .insert_resource<sf::RenderWindow>(sf::VideoMode(500, 500), "SFML Works!")
      .add_startup_system(spawn_circle)
      .add_system(speed_system)  // by default adds it to the `Update` stage
      .add_system_to_stage<stages::PostUpdate>(draw_circle)
      .add_system_to_stage<stages::PostUpdate>(exit_game)
      .run();
}
