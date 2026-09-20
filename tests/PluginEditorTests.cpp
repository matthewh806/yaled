#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
    template <typename ControlType>
    ControlType* findControl (juce::AudioProcessorEditor& editor, const char* parameterId)
    {
        return dynamic_cast<ControlType*> (editor.findChildWithID (parameterId));
    }

    float rawValue (PluginProcessor& plugin, const char* parameterId)
    {
        return plugin.apvts.getRawParameterValue (parameterId)->load();
    }
}

TEST_CASE ("PluginEditor has a control for every plugin parameter", "[PluginEditor]")
{
    PluginProcessor plugin;
    std::unique_ptr<juce::AudioProcessorEditor> editor (plugin.createEditor());

    REQUIRE_FALSE (plugin.getParameters().isEmpty());

    // Walks the processor's real parameter list, so a parameter added later without a
    // control in the editor fails here.
    for (auto* parameter : plugin.getParameters())
    {
        auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (parameter);
        REQUIRE (withId != nullptr);

        INFO ("parameter: " << withId->paramID);
        CHECK (editor->findChildWithID (withId->paramID) != nullptr);
    }
}

TEST_CASE ("PluginEditor knobs drive their parameters", "[PluginEditor]")
{
    PluginProcessor plugin;
    std::unique_ptr<juce::AudioProcessorEditor> editor (plugin.createEditor());

    auto* chunkLength = findControl<juce::Slider> (*editor, "chunkLengthMs");
    auto* mix = findControl<juce::Slider> (*editor, "mix");
    auto* feedback = findControl<juce::Slider> (*editor, "feedback");
    REQUIRE (chunkLength != nullptr);
    REQUIRE (mix != nullptr);
    REQUIRE (feedback != nullptr);

    chunkLength->setValue (400.0, juce::sendNotificationSync);
    mix->setValue (75.0, juce::sendNotificationSync);
    feedback->setValue (30.0, juce::sendNotificationSync);

    CHECK (rawValue (plugin, "chunkLengthMs") == Catch::Approx (400.0f));
    CHECK (rawValue (plugin, "mix") == Catch::Approx (75.0f));
    CHECK (rawValue (plugin, "feedback") == Catch::Approx (30.0f));
}

TEST_CASE ("PluginEditor tempo sync controls drive their parameters", "[PluginEditor]")
{
    PluginProcessor plugin;
    std::unique_ptr<juce::AudioProcessorEditor> editor (plugin.createEditor());

    auto* sync = findControl<juce::Button> (*editor, "tempoSync");
    auto* division = findControl<juce::ComboBox> (*editor, "tempoSyncDivision");
    REQUIRE (sync != nullptr);
    REQUIRE (division != nullptr);

    REQUIRE (rawValue (plugin, "tempoSync") == 0.0f);
    sync->setToggleState (true, juce::sendNotificationSync);
    CHECK (rawValue (plugin, "tempoSync") == 1.0f);

    division->setSelectedItemIndex (3, juce::sendNotificationSync); // 1/8
    CHECK (rawValue (plugin, "tempoSyncDivision") == 3.0f);
}

TEST_CASE ("PluginEditor offers every tempo sync division the parameter has", "[PluginEditor]")
{
    PluginProcessor plugin;
    std::unique_ptr<juce::AudioProcessorEditor> editor (plugin.createEditor());

    auto* division = findControl<juce::ComboBox> (*editor, "tempoSyncDivision");
    auto* parameter = dynamic_cast<juce::AudioParameterChoice*> (plugin.apvts.getParameter ("tempoSyncDivision"));
    REQUIRE (division != nullptr);
    REQUIRE (parameter != nullptr);

    REQUIRE (division->getNumItems() == parameter->choices.size());
    for (int i = 0; i < parameter->choices.size(); ++i)
        CHECK (division->getItemText (i) == parameter->choices[i]);
}

TEST_CASE ("PluginEditor shows the parameter values it is opened with", "[PluginEditor]")
{
    PluginProcessor plugin;
    *dynamic_cast<juce::AudioParameterFloat*> (plugin.apvts.getParameter ("mix")) = 20.0f;
    *dynamic_cast<juce::AudioParameterBool*> (plugin.apvts.getParameter ("tempoSync")) = true;

    std::unique_ptr<juce::AudioProcessorEditor> editor (plugin.createEditor());

    auto* mix = findControl<juce::Slider> (*editor, "mix");
    auto* sync = findControl<juce::Button> (*editor, "tempoSync");
    REQUIRE (mix != nullptr);
    REQUIRE (sync != nullptr);

    CHECK (mix->getValue() == Catch::Approx (20.0));
    CHECK (sync->getToggleState());
}
