class Makexx < Formula
  desc "C++ build system generator — write makefile.cpp, get GNU makefiles"
  homepage "https://github.com/abdul900/makexx"
  url "https://github.com/abdul900/makexx/archive/refs/tags/VERSION.tar.gz"
  # sha256 "UPDATE_ON_RELEASE"
  license "MIT"
  head "https://github.com/abdul900/makexx.git", branch: "main"

  depends_on "cmake" => :build

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
