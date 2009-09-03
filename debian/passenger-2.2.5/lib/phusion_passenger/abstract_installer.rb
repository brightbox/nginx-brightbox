#  Phusion Passenger - http://www.modrails.com/
#  Copyright (c) 2008, 2009 Phusion
#
#  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.

require 'phusion_passenger/constants'
require 'phusion_passenger/packaging'
require 'phusion_passenger/console_text_template'

module PhusionPassenger

# Abstract base class for installers. Used by passenger-install-apache2-module
# and passenger-install-nginx-module.
class AbstractInstaller
	PASSENGER_WEBSITE = "http://www.modrails.com/"
	PHUSION_WEBSITE = "www.phusion.nl"

	def initialize(options = {})
		options.each_pair do |key, value|
			instance_variable_set(:"@#{key}", value)
		end
	end
	
	def start
		install!
	ensure
		reset_terminal_colors
	end

private
	def reset_terminal_colors
		STDOUT.write("\e[0m")
		STDOUT.flush
	end
	
	def color_print(text)
		STDOUT.write(ConsoleTextTemplate.new(:text => text).result)
		STDOUT.flush
	end
	
	def color_puts(text)
		color_print("#{text}\n")
	end
	
	def render_template(name, options = {})
		puts ConsoleTextTemplate.new({ :file => name }, options).result
	end
	
	def new_screen
		puts
		line
		puts
	end
	
	def line
		puts "--------------------------------------------"
	end
	
	def prompt(message)
		done = false
		while !done
			color_print "#{message}: "
			begin
				result = STDIN.readline
			rescue EOFError
				exit 2
			end
			result.strip!
			done = !block_given? || yield(result)
		end
		return result
	rescue Interrupt
		exit 2
	end
	
	def wait(timeout = nil)
		return if @auto
		begin
			if timeout
				require 'timeout' unless defined?(Timeout)
				begin
					Timeout.timeout(timeout) do
						STDIN.readline
					end
				rescue Timeout::Error
					# Do nothing.
				end
			else
				STDIN.readline
			end
		rescue Interrupt
			exit 2
		end
	end
	
	def sh(*args)
		puts "# #{args.join(' ')}"
		result = system(*args)
		if result
			return true
		elsif $?.signaled? && $?.termsig == Signal.list["INT"]
			raise Interrupt
		else
			return false
		end
	end
	
	def dependencies
		return []
	end
	
	def check_dependencies
		new_screen
		missing_dependencies = []
		color_puts "<banner>Checking for required software...</banner>"
		puts
		dependencies.each do |dep|
			color_print " * #{dep.name}... "
			result = dep.check
			if result.found?
				if result.found_at
					color_puts "<green>found at #{result.found_at}</green>"
				else
					color_puts "<green>found</green>"
				end
			else
				color_puts "<red>not found</red>"
				missing_dependencies << dep
			end
		end
		
		if missing_dependencies.empty?
			return true
		else
			puts
			color_puts "<red>Some required software is not installed.</red>"
			color_puts "But don't worry, this installer will tell you how to install them.\n"
			color_puts "<b>Press Enter to continue, or Ctrl-C to abort.</b>"
			if PhusionPassenger.natively_packaged?
				wait(10)
			else
				wait
			end
			
			line
			puts
			color_puts "<banner>Installation instructions for required software</banner>"
			puts
			missing_dependencies.each do |dep|
				print_dependency_installation_instructions(dep)
				puts
			end
			color_puts "If the aforementioned instructions didn't solve your problem, then please take"
			color_puts "a look at the Users Guide:"
			puts
			color_puts "  <yellow>#{users_guide}</yellow>"
			return false
		end
	end
	
	def print_dependency_installation_instructions(dep)
		color_puts " * To install <yellow>#{dep.name}</yellow>:"
		if !dep.install_command.nil?
			color_puts "   Please run <b>#{dep.install_command}</b> as root."
		elsif !dep.install_instructions.nil?
			color_puts "   " << dep.install_instructions
		elsif !dep.website.nil?
			color_puts "   Please download it from <b>#{dep.website}</b>"
			if !dep.website_comments.nil?
				color_puts "   (#{dep.website_comments})"
			end
		else
			color_puts "   Search Google."
		end
	end
end

end # module PhusionPassenger
