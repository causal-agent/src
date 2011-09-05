#!/usr/bin/env ruby

require 'fileutils'

dotfiles = {}

dirs = ['.']
dirs.each do |dir|
  Dir.foreach(dir) do |file|
    next if file[0] == '.'
    file = File.join(dir, file)
    if File.directory? file
      dirs << file
      next
    end
    dotfiles[file] = file.sub('.', '~').sub('_', '.') if %r{/_} =~ file
  end
end

dotfiles.each do |a, b|
  a, b = File.expand_path(a), File.expand_path(b)
  FileUtils.cp(b, a)
  puts "#{b} -> #{a}"
end

system('git add .')
system('git commit')
system('git push')
