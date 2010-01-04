=begin 
Copyright (C) 2009 Ricardo Pescuma Domenecci

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
=end

require 'mechanize'

class GoogleCode
  FEATURED = "Featured"
  DEPRECATED = "Deprecated"

  def initialize(project, username, password)
    @browser = WWW::Mechanize.new
    @project = project

    page = @browser.get(build_url)

    page = @browser.click page.link_with(:text => 'Sign in')

    form = page.forms.find_all { |form| \
        form.action == 'https://www.google.com/accounts/ServiceLoginAuth?service=code' \
        && form.has_field?("Email") \
        && form.has_field?("Passwd") }[0]

    raise "Could not talk to googlecode. Have they changed the pages layout?" unless form

    form.Email = username
    form.Passwd = password

    page = form.submit

    raise 'GoogleCode: Invalid username/password' unless page.forms.length == 0
  end

  def all_files
    get_files("downloads/list?can=1")
  end

  def current_files
    get_files("downloads/list?can=2")
  end

  def featured_files
    get_files("downloads/list?can=3")
  end

  def deprecated_files
    get_files("downloads/list?can=4")
  end

  def fetch(filename)
    page = @browser.get(build_url("downloads/detail?name=#{filename}"))

    form = page.form_with(:action => "update.do?name=#{filename}")
    raise "Could not talk to googlecode. Have they changed the pages layout?" unless form

    file = Download.new filename
    file.description = form.summary
    file.labels = []
    form.fields.each { |f| file.labels << f.value if f.name == 'label' && f.value.length > 0 }

    file
  end

  def update(file)
    page = @browser.get(build_url("downloads/detail?name=#{file.filename}"))

    form = page.form_with(:action => "update.do?name=#{file.filename}")
    raise "Could not talk to googlecode. Have they changed the pages layout?" unless form

    form.summary = file.description

    pos = 0
    form.fields.each do |f|
      if f.name == 'label'
        f.value = file.labels[pos] || ''
        pos = pos + 1
      end
    end

    form.submit # TODO Validate result
  end

  def upload(file)
    raise 'filename can\'t be empty' unless (file.filename || '').strip.length > 0
    raise 'description can\'t be empty' unless (file.description || '').strip.length > 0
    raise 'local_file can\'t be empty' unless (file.local_file || '').strip.length > 0

    local_file = File.expand_path(file.local_file)
    raise 'Local file could not be read' unless File.exist?(local_file)
    raise 'Filename should be equal to local filename' unless File.basename(local_file) == file.filename

    page = @browser.get(build_url('downloads/entry'))

    form = page.form_with(:action => "http://uploads.code.google.com/upload/#{@project}")
    raise "Could not talk to googlecode. Have they changed the pages layout?" unless form

    form.summary = file.description.strip

    pos = 0
    form.fields.each do |f|
      if f.name == 'label'
        f.value = (file.labels[pos] || '').strip
        pos = pos + 1
      end
    end

    form.file_uploads.first.file_name = local_file

    page = form.submit

    form = page.form_with(:action => "http://uploads.code.google.com/upload/#{@project}")
    raise "Error uploading file #{file}" if form

  end

  class Download
    attr_accessor :description, :labels, :local_file
    attr_reader :filename

    def initialize(filename, fetcher = nil)
      @filename = filename.strip
      raise 'filename can\'t be empty' unless @filename
      
      @description = ''
      @labels = []
      @fetcher = fetcher
    end

    def description
      fetch
      return @description
    end
    
    def labels
      fetch
      return @labels
    end

    def to_s
      fetch
      ret = @filename
      ret += ' : ' + @description if @description
      ret += ' [' + @labels.join(', ') + ']' if @labels.length > 0
    end

    def <=>(other)
      return @filename.casecmp(other.filename)
    end

    private

    def fetch
      return unless @fetcher

      tmp = @fetcher.fetch @filename

      @description = tmp.description
      @labels = tmp.labels
        
      @fetcher = nil
    end

  end


  private

  def build_url(suffix = '')
    if suffix == ''
      return "http://code.google.com/p/#{@project}"
    elsif suffix[0] == '/'
      "http://code.google.com/p/#{@project}#{suffix}"
    else
      "http://code.google.com/p/#{@project}/#{suffix}"
    end
  end

  def get_files(url)
    page = @browser.get(build_url(url))

    start = "http://#{@project}.googlecode.com/files/"

    ret = []
    page.links.each do |link|
      id = link.href
      if id.length > start.length && id.slice(0, start.length) == start
        id = id.slice(start.length, id.length - start.length)
        ret << Download.new(id, self)
      end
    end

    return ret
  end

end
