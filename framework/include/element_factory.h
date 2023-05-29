//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#ifndef SOPHON_STREAM_FRAMEWORK_ELEMENT_FACTORY_H_
#define SOPHON_STREAM_FRAMEWORK_ELEMENT_FACTORY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "common/ErrorCode.h"
#include "common/Singleton.hpp"
#include "element.h"

namespace sophon_stream {
namespace framework {

/**
 * Element工厂
 */
class ElementFactory {
 public:
  using ElementMaker = std::function<std::shared_ptr<framework::Element>()>;

  /**
   * 添加Element产生器
   * @param[in] elementName:
   * @param[in] elementMaker:
   */
  common::ErrorCode addElementMaker(const std::string& elementName,
                                    ElementMaker elementMaker);
  /**
   * 移除Element产生器
   * @param[in] elementName:
   * @param[in] elementMaker:
   */
  // void removeElementMaker(const std::string& elementName);

  /**
   * 获取产生器
   * @param[in] elementName:
   */
  std::shared_ptr<framework::Element> make(const std::string& elementName);

  friend class common::Singleton<ElementFactory>;

  /**
   * Constructor of class ElementFactory.
   */
  ElementFactory();

  std::map<std::string, ElementMaker> mElementMakerMap;
  /**
   * Destructor of class ElementFactory.
   */
  ~ElementFactory();

  ElementFactory(const ElementFactory&) = delete;
  ElementFactory& operator=(const ElementFactory&) = delete;
  ElementFactory(ElementFactory&&) = delete;
  ElementFactory& operator=(ElementFactory&&) = delete;
};

using SingletonElementFactory = common::Singleton<ElementFactory>;

#define REGISTER_WORKER(elementName, ElementClass)                            \
  struct ElementClass##Register {                                             \
    ElementClass##Register() {                                                \
      auto& elementFactory =                                                  \
          ::sophon_stream::framework::SingletonElementFactory::getInstance(); \
      std::cout << elementName << std::endl;                                  \
      elementFactory.addElementMaker(                                         \
          elementName, []() { return std::make_shared<ElementClass>(); });    \
    }                                                                         \
  };                                                                          \
  static ElementClass##Register g##ElementClass##Register;

}  // namespace framework
}  // namespace sophon_stream

#endif  // SOPHON_STREAM_FRAMEWORK_ELEMENT_FACTORY_H_