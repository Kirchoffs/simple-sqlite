RSpec.configure do |config|
    config.before(:suite) do
        RSpec::Support::ObjectFormatter.default_instance.max_formatted_output_length = nil
    end
end
