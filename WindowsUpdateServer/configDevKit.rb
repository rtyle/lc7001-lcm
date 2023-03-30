require 'yaml'

module DevKitInstaller

  CONFIG_FILE = 'config.yml'

  def self.usage
<<-EOT

Configures an MSYS/MinGW based Development Kit (DevKit) for
each of the Ruby installations on your Windows system. The
DevKit enables you to build many of the available native
RubyGems that don't yet have a binary gem.

Usage: ruby dk.rb COMMAND [options]

where COMMAND is one of:

  init     prepare DevKit for installation

EOT
  end

  def self.init
    # Hardcode Ruby installation path
    ir = []
    ir[0] = File.expand_path(File.dirname(__FILE__)) + '/ruby-2.2.4-i386-mingw32'
    puts 'Ruby installs %s' % ir[0]
    puts <<-EOT

Initialization complete! Please review and modify the auto-generated
'config.yml' file to ensure it contains the root directories to all
of the installed Rubies you want enhanced by the DevKit.
EOT

    File.open(CONFIG_FILE, 'w') do |f|
      f.write <<-EOT
# This configuration file contains the absolute path locations of all
# installed Rubies to be enhanced to work with the DevKit. This config
# file is generated by the 'ruby dk.rb init' step and may be modified
# before running the 'ruby dk.rb install' step. To include any installed
# Rubies that were not automagically discovered, simply add a line below
# the triple hyphens with the absolute path to the Ruby root directory.
#
# Example:
#
# ---
# - C:/ruby19trunk
# - C:/ruby192dev
#
EOT
      unless ir.empty? then f.write(ir.to_yaml) else f.write("---\n") end
    end
  end
  private_class_method :init

  def self.usage_and_exit
    $stderr.puts usage
    exit(-1)
  end

  def self.run(*args)
    send(args.first)
  end

end

if __FILE__ == $0
  if ARGV.empty? || ARGV.delete('--help') || ARGV.delete('-h')
    DevKitInstaller.usage_and_exit
  end

  cmd = ARGV.delete('init')

  DevKitInstaller.usage_and_exit unless ARGV.empty?
  DevKitInstaller.run(cmd)
end