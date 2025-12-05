-- Convert a purely cobblestone farm wall into a randomized block vomit mess 
-- farm wall.

local blockTypes = {
  {
    blocks.cobblestone, 
    blocks.cobblestone, 
    blocks.andesite, 
    blocks.cracked_stone_bricks,
    blocks.dead_tube_coral_block
  },
  {
    blocks.cobblestone_slab, 
    blocks.andesite_slab, 
    blocks.stone_slab,
    blocks.stone_brick_slab
  },
  {
    blocks.cobblestone_stairs, 
    blocks.andesite_stairs, 
    blocks.stone_stairs,
    blocks.stone_brick_stairs
  }
}

function isWallAbove() 
  local current = getBlock(x,y,z)
  if current ~= blocks.dirt then
    return 0
  end

  for i = 1, 3 do
    local tidx = getWallTypeIdx(getBlock(x, y + i, z))
    if tidx ~= 0 then
      return 1
    end
  end

  return 0
end

function getWallTypeIdx(b) 
  for i = 1, #blockTypes do
    for j = 1, #blockTypes[i] do
      if b == blockTypes[i][j] then
        return i
      end
    end
  end
  return 0
end

local typeIdx = getWallTypeIdx(getBlock(x,y,z))
local aboveType = isWallAbove()

if aboveType ~= 0 then
  typeIdx = 1
elseif typeIdx == 0 then 
  return nil
end

local len = #blockTypes[typeIdx]
local noise = math.floor(getSimplexNoise(x, y, z) * len + 1)

local type = blockTypes[typeIdx][noise]
local cb = getBlockState(x,y,z)

if typeIdx == 2 then
  local slabType = getBlockProperty(cb, "type")
  type = withBlockProperty(type, "type="..slabType)
elseif typeIdx == 3 then
  local facing = getBlockProperty(cb, "facing")
  local half = getBlockProperty(cb,  "half")
  local shape = getBlockProperty(cb, "shape")
  type = withBlockProperty(type, "facing="..facing, "half="..half, "shape="..shape)
end

return type
