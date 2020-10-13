//
// Created by sea on 12/22/18.
//
// 2018. SEA
//

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "drcomclient/application.h"
#include "resources.h" //glib-compile-resources --target=../resources.h --generate-source gresource.xml

void init_log();

int main(int argc, char **argv) {
  init_log();

  auto app = DrcomClient::Application::create();
  return app->run(argc, argv);
}

void init_log() {
  std::string home = getenv("HOME");
  auto &&path = home + "/drcomclient.log";
  auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(path);
  std::initializer_list<spdlog::sink_ptr> sinks{stdout_sink, file_sink};
  auto logger = std::make_shared<spdlog::logger>("drcom", sinks);
  logger->set_level(spdlog::level::debug);
  spdlog::register_logger(logger);
}