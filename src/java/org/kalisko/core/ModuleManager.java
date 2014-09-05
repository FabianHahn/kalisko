/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2014, Kalisko Project Leaders
 * Copyright (c) 2014, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *         in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.kalisko.core;

import java.util.HashMap;
import java.util.Map;

/** Manages the life cycles of module instances. This class is *not* thread safe. */
public class ModuleManager {
  private Map<Class<? extends Module>, ModuleState> moduleStates;

  /** This default constructor is called by the native code through JNI. */
  public ModuleManager() {
    moduleStates = new HashMap<>();
  }

  /**
   * Instantiates and runs the specified module. The module must have a default constructor and
   * must not already be executing.
   * @param className the fully qualified class name of the module, e.g.,
   *    org.kalisko.modules.demo.DemoModule.
   * @return whether or not the module has been loaded successfully
   */
  public boolean executeModule(String className) {
    // TODO: Switch to proper Loggers instead of printing to stdout.
    System.out.println("Executing module: " + className);

    // Resolve the class by name.
    final Class<?> moduleClass;
    try {
      moduleClass = Class.forName(className);
    } catch (ClassNotFoundException e) {
      e.printStackTrace();
      return false;
    }

    // Check that this module is not already being executed.
    if (moduleStates.containsKey(moduleClass)) {
      System.out.println("Already executed module of class: " + moduleClass.getName());
      return false;
    }

    // Instantiate the module.
    final Module module;
    try {
      module = (Module) moduleClass.newInstance();
    } catch (IllegalAccessException | InstantiationException e) {
      e.printStackTrace();
      return false;
    }

    // Attempt to initialize the module.
    try {
      module.initialize();
    } catch (Throwable t) {
      t.printStackTrace();
      return false;
    }

    // Run the module's execute method in a separate thread.
    Thread thread = new Thread(new Runnable() {
      @Override
      public void run() {
        try {
          module.run();
        } catch (Throwable t) {
          System.out.println("Uncaught execption in module.run(). Aborting.");
          t.printStackTrace();
        }
      }
    });
    moduleStates.put(module.getClass(), new ModuleState(module, thread));
    thread.run();

    return true;
  }

  private static class ModuleState {
    @SuppressWarnings("unused")  // TODO: remove suppression when building cleanup.
    public final Module module;

    @SuppressWarnings("unused")  // TODO: remove suppression when building cleanup.
    public final Thread thread;

    private ModuleState(Module module, Thread thread) {
      this.module = module;
      this.thread = thread;
    }
  }
}
