#include <SFML/Graphics.hpp>
#include <random>

#include "nova/nova.hpp"

using namespace nova;

// A simple component
struct velocity {
  float dx{};
  float dy{};
};

struct acceleration {
  float ddx{};
  float ddy{};
};

// A Bundle of components
struct CircleBundle {
  using is_bundle = void;

  velocity vel{};
  sf::CircleShape circle{};
};

// make a random number between a (min, max)
auto rng(const float min, const float max) -> float {
  static std::random_device rd{};
  static std::mt19937 gen(rd());
  std::uniform_real_distribution<float> distrib(min, max);
  return distrib(gen);
}

// a system to spawn circles
auto spawn_circles(Resource<sf::RenderWindow> window, Registry& registry)
    -> void {
  const auto size = window->getSize();
  const auto x_center = (float)size.x / 2.f;
  const auto y_center = (float)size.y / 2.f;

  for (auto i = 0; i < 10; ++i) {
    const auto e = registry.create();

    auto circle = sf::CircleShape(rng(5.f, 20.f));
    circle.setFillColor(sf::Color::Cyan);
    circle.setPosition(sf::Vector2f{x_center, y_center});

    registry.emplace_bundle(e, CircleBundle{.vel =
                                                velocity{
                                                    .dx = rng(-20.f, 20.f),
                                                    .dy = rng(-20.f, 20.f),
                                                },
                                            .circle = circle});

    // only add acceleration to half of the circles
    if (i % 2 == 0) {
      registry.emplace<acceleration>(e, acceleration{
                                            .ddx = rng(-5.f, 5.f),
                                            .ddy = rng(-5.f, 5.f),
                                        });
    }
  }
}

auto acceleration_system(Resource<const Time> time,
                         View<With<const acceleration, velocity>> view)
    -> void {
  const auto delta = time->delta_seconds<float>();
  for (auto [_, acc, vel] : view.each()) {
    vel.dx += acc.ddx * delta;
    vel.dy += acc.ddy * delta;
  }
}

auto speed_system(Resource<const Time> time,
                  View<With<const velocity, sf::CircleShape>> view) -> void {
  const auto delta = time->delta_seconds<float>();
  for (auto [_, velocity, circle] : view.each()) {
    const auto [x, y] = circle.getPosition();

    const auto new_x = x + velocity.dx * delta;
    const auto new_y = y + velocity.dy * delta;
    circle.setPosition(sf::Vector2f{new_x, new_y});
  }
}

auto draw_circle(Resource<sf::RenderWindow> window,
                 View<With<const sf::CircleShape>> view) -> void {
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
      .add_startup_system(spawn_circles)
      .add_system(system(speed_system).label("speed"))
      .add_system(system(acceleration_system).before("speed"))
      .add_system_to_stage<stages::PostUpdate>(draw_circle)
      .add_system_to_stage<stages::Last>(exit_game)
      .run();
}
