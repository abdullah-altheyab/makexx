class Makexx < Formula
  desc "C++ build system generator — write makefile.cpp, get GNU makefiles"
  homepage "https://github.com/abdullah-altheyab/makexx"
  url "https://github.com/abdullah-altheyab/makexx/archive/refs/tags/VERSION.tar.gz"
  # sha256 "UPDATE_ON_RELEASE"
  license "MIT"
  head "https://github.com/abdullah-altheyab/makexx.git", branch: "main"

  depends_on "cmake" => :build

  # Runtime requirements: `make` to run the generated makefile and a C++
  # compiler to build the user's `makefile.cpp`. macOS users get both via
  # Xcode Command Line Tools; Linuxbrew users get make explicitly here.
  # `gcc` provides g++ — the first compiler `makexx` probes by default —
  # without forcing it as the system compiler.
  depends_on "make"
  depends_on "gcc"

  def install
    system "cmake", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    mkdir "test" do
      system bin/"makexx", "-c"
      assert_predicate Pathname.pwd/"makefile", :exist?
    end
  end
end
