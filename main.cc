//
// Created by sea on 12/22/18.
//
// 2018. SEA
//

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "drcomclient/application.h"
#include "resources.h" //glib-compile-resources --target=../include/resources.h --generate-source gresource.xml

int main(int argc, char **argv) {
  //register logger
  auto path = getenv("HOME") + std::string("/drcomclient.log");
  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
  sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(path));
  auto &&logger = std::make_shared<spdlog::logger>("drcom", sinks.begin(), sinks.end());
  logger->set_level(spdlog::level::debug);
  spdlog::register_logger(logger);

  //start an application
  drcomclient::Application app;
  return app.run(argc, argv);
}