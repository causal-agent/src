#!/usr/bin/env ruby
require 'cgi'
require 'json'
require 'mastodon'

require_relative 'secret'

cgi = CGI.new
begin
	payload = JSON.parse(cgi.params['payload'].first)
rescue
	cgi.out('status' => 'BAD_REQUEST', 'type' => 'text/plain') { 'bad' }
	exit
end
if payload['secret'] != GITEA_SECRET
	cgi.out('status' => 'FORBIDDEN', 'type' => 'text/plain') { 'no' }
	exit
end

client = Mastodon::REST::Client.new(
	base_url: MASTODON_URL,
	bearer_token: MASTODON_TOKEN,
)

payload['commits'].reverse.each do |commit|
	message = commit['message']
		.split("\n\n")
		.map {|p| p.split("\n").join(' ') }
		.join("\n")
	client.create_status("🚽 #{message}\n#{commit['url']}")
end

cgi.out('text/plain') { 'ok' }
