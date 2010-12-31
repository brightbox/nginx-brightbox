require 'support/config'
require 'support/test_helper'
require 'phusion_passenger/application'
include PhusionPassenger

describe Application do
	include TestHelper
	
	before :each do
		@stub = setup_rails_stub('foobar')
	end
	
	after :each do
		teardown_rails_stub
	end
	
	it "correctly detects Rails version numbers specified in environment.rb" do
		rails_version = Application.detect_framework_version(@stub.app_root)
		rails_version.should =~ /^2\.0\.(\d+)$/
	end
	
	it "returns :vendor if an application uses a vendored Rails" do
		@stub.use_vendor_rails('minimal')
		rails_version = Application.detect_framework_version(@stub.app_root)
		rails_version.should == :vendor
	end
	
	it "returns nil if an application does not specify its Rails version" do
		File.write(@stub.environment_rb) do |content|
			content.sub(/^RAILS_GEM_VERSION = .*$/, '')
		end
		rails_version = Application.detect_framework_version(@stub.app_root)
		rails_version.should be_nil
	end
	
	it "raises VersionNotFound if a nonexistant Rails version is specified" do
		File.write(@stub.environment_rb) do |content|
			content.sub(/^RAILS_GEM_VERSION = .*$/, "RAILS_GEM_VERSION = '1.9.1972'")
		end
		detector = lambda { Application.detect_framework_version(@stub.app_root) }
		detector.should raise_error(::PhusionPassenger::VersionNotFound)
	end
end
