require 'sinatra'
configure :production do
end

# file info dictionary
$dict = {
  "1-0-0-1-abcde-LCM1" => {
    "filename"      => "1-0-0-1-abcde-LCM1.s19",
    "version"       => "1.0.0-1-abcde",
    "date"          => "2023-03-31",
    "branch"        => "dev",
    "notes"         => "TEST NOTES",
    "lines"         => [],
    "signatureLines"=> [],
  }
}

# load branches to memory (one per branch)
$dict.each do |branch, info| 
  info["lines"] = IO.readlines(info["filename"])
  begin
    info["signatureLines"] = IO.readlines(branch + ".signature")
  rescue
    # file probably doesn't exist
  end
end

# get file version
def version(b)
  vn = $dict[b] ? $dict[b]["version"] : nil
  v = vn ? vn : nil
  return v
end

# get file date
def date(b)
  da = $dict[b] ? $dict[b]["date"] : nil
  d = da ? da : nil
  return d
end

# get file branch
def branch(b)
  ba = $dict[b] ? $dict[b]["branch"] : nil
  b = ba ? ba : nil
  return b
end

# get file notes
def notes(b)
  na = $dict[b] ? $dict[b]["notes"] : nil
  n = na ? na : nil
  return n
end

# get the key for the provided branch
def getKey(b)
   #Filename is the name of the branch with .s19 appended
   filename = "#{b}.s19"

   begin
      key = File.basename(File.readlink(filename), ".s19")
   rescue NotImplementedError
      # readlink is not implemented so it must be an actual branch
      key = b
   rescue Errno::EINVAL
      # Not a symlink so it must be an actual branch. Set the key to the branch name
      key = b
   rescue Errno::ENOENT
      # File does not exist. Set the key to invalid
      key = nil
   end

  return key
end

# get the key for the provided branch
def getKeySignature(b)
   #Filename is the name of the branch with .s19 appended
   filename = "#{b}.signature"

   begin
      key = File.basename(File.readlink(filename), ".signature")
   rescue NotImplementedError
      # readlink is not implemented so it must be an actual branch
      key = b
   rescue Errno::EINVAL
      # Not a symlink so it must be an actual branch. Set the key to the branch name
      key = b
   rescue Errno::ENOENT
      # File does not exist. Set the key to invalid
      key = nil
   end

  return key
end

# get the file by line number
get '/:branch/line/:line' do |b, l|
  return "Invalid Chunk Number: Chunks are 1 indexed!" if l.to_i < 1
  key = getKey(b)
  return "Invalid Branch" if not $dict[key]
  li = $dict[key]["lines"][l.to_i - 1]
  li ? (l + ":" + li + "\x1E") : (l + ":EOF\x1E")
end

# get file by chunk size
get '/:branch/chunk/:size/:num' do |b, s, n|
  return "Invalid Chunk Size: Must be at least 1!" if s.to_i < 1
  return "Invalid Chunk Number: Chunks are 1 indexed!" if n.to_i < 1
  key = getKey(b)
  return "Invalid Branch" if not $dict[key]
  r = ""
  lines = *(((n.to_i-1)*(s.to_i))..((n.to_i)*(s.to_i))).to_a
  lines.shift
  s.to_i.times{ln = lines.shift; l = $dict[key]["lines"][ln - 1]; l ? r += (ln.to_s + ":" + l.to_s) : r += (ln.to_s + ":EOF\n")}
  r += "\x1E"
  r
end

# get the whole file
get '/:branch/whole' do |b|
  key = getKey(b)
  return "Invalid Branch" if not $dict[key]
  $dict[key]["lines"]
end

# get info on file
get '/:branch/info' do |b|
  key = getKey(b)
  return "Invalid Branch" if not $dict[key]
  v = version(key)
  d = date(key)
  branch = branch(key)
  notes = notes(key)
  "L(%d) V(%s) D(%s) B(%s) N(%s)\x1E" % [$dict[key]["lines"].count, v, d, branch, notes]
end

# get info on file
get '/:branch/signature' do |b|
    key = getKeySignature(b)
    return "Invalid Branch" if not $dict[key]
    $dict[key]["signatureLines"]
end

