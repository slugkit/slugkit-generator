#
# SlugKit Flutter FFI plugin — macOS.
# Vendors a prebuilt dynamic Slugkit.xcframework (built from the userver-free C ABI).
#
Pod::Spec.new do |s|
  s.name             = 'slugkit'
  s.version          = '0.1.0'
  s.summary          = 'SlugKit human-readable ID generator (native engine).'
  s.description      = 'Dart/Flutter FFI plugin over the SlugKit generator C ABI.'
  s.homepage         = 'https://slugkit.dev'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'SlugKit' => 'dev@slugkit.dev' }

  s.source           = { :path => '.' }
  # Build the dynamic xcframework if it is not already present (idempotent).
  s.prepare_command  = 'bash ../scripts/build-xcframework.sh'
  s.vendored_frameworks = 'Slugkit.xcframework'

  s.dependency 'FlutterMacOS'
  s.platform = :osx, '10.13'
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
  s.swift_version = '5.0'
end
